#include "app.hpp"

namespace hw18 {

App::App(SPI_HandleTypeDef* spi, UART_HandleTypeDef* uart, TIM_HandleTypeDef* tim,
         IWDG_HandleTypeDef* iwdg, GPIO_TypeDef* accel_cs_port, std::uint16_t accel_cs_pin,
         GPIO_TypeDef* led_port, std::uint16_t led_pin)
    : spi_(spi),
      uart_(uart),
      tim_(tim),
      iwdg_(iwdg),
      led_port_(led_port),
      led_pin_(led_pin),
      accel_(spi, accel_cs_port, accel_cs_pin) {}

void App::send_line(const char* text, std::size_t len) {
  HAL_UART_Transmit(uart_, reinterpret_cast<const std::uint8_t*>(text), static_cast<std::uint16_t>(len),
                     100);
}

void App::apply_period(std::uint32_t period_ms) {
  period_ms_ = clamp_period_ms(period_ms);
  // PSC у main() так, що один крок TIM це 1 мс
  // ARR - період у мс мінус один
  __HAL_TIM_SET_AUTORELOAD(tim_, period_ms_ - 1);
  __HAL_TIM_SET_COUNTER(tim_, 0);

  char ack[32];
  std::size_t len = format_period_ack(ack, sizeof(ack), period_ms_);
  if (len > 0) send_line(ack, len);
}

void App::apply_mode(Mode mode) {
  mode_ = mode;
  char ack[32];
  std::size_t len = format_mode_ack(ack, sizeof(ack), mode_);
  if (len > 0) send_line(ack, len);
}

void App::begin() {
  bool ok = accel_.init();
  const char* banner = ok ? "hw18 ready: lis3dsh ok\r\n"
                           : "hw18 ready: lis3dsh WHO_AM_I mismatch (older LIS302DL board?)\r\n";
  send_line(banner, __builtin_strlen(banner));
  apply_period(period_ms_);
  apply_mode(mode_);
}

void App::handle_tick() {
  Lis3dsh::Reading r = accel_.read();

  Sample sample{
      .timestamp_ms = HAL_GetTick(),
      .ax = r.ax_g,
      .ay = r.ay_g,
      .az = r.az_g,
      .mode = mode_,
  };
  char line[96];
  std::size_t len = format_report(line, sizeof(line), sample);
  if (len > 0) send_line(line, len);

  HAL_GPIO_TogglePin(led_port_, led_pin_);  // led блимає в такт
}

void App::handle_button() {
  std::uint32_t now = HAL_GetTick();
  if (now - last_button_ms_ < kButtonDebounceMs) return;  // брязкіт
  last_button_ms_ = now;

  Mode next = (mode_ == Mode::Slow) ? Mode::Fast : Mode::Slow;
  apply_mode(next);
  apply_period(next == Mode::Fast ? kFastPeriodMs : kSlowPeriodMs);
}

void App::handle_uart_commands() {
  char line[LineAssembler::kCapacity + 1];
  while (rx_.take_line(line, sizeof(line)) > 0) {
    Command cmd = parse_command(line);
    switch (cmd.kind) {
      case Command::Kind::Period:
        apply_period(cmd.period_ms);
        break;
      case Command::Kind::Mode:
        apply_mode(cmd.mode);
        break;
      case Command::Kind::Unknown: {
        static constexpr char kUnknown[] = "err: unknown command\r\n";
        send_line(kUnknown, sizeof(kUnknown) - 1);
        break;
      }
      case Command::Kind::None:
      default:
        break;
    }
  }
}

void App::poll() {
  if (tick_pending_) {
    tick_pending_ = false;
    handle_tick();
  }
  if (button_pending_) {
    button_pending_ = false;
    handle_button();
  }
  handle_uart_commands();

  if (iwdg_ != nullptr) HAL_IWDG_Refresh(iwdg_); 
}

}  // namespace hw18
