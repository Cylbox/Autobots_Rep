#include <iostream>
#include <chrono>
#include <thread>
#include <atomic>
#include <mutex>
#include <string>

enum class NetworkState
{
  RELEASED,
  REPEAT_MESSAGE,
  OPERATIONAL
};

// Клас UDP Network Management, що моделює логіку AUTOSAR UDP NM
class UdpNm
{
private:
  std::atomic<NetworkState> state{NetworkState::RELEASED};
  std::atomic<bool> stopTimers{false};

  int immediateTransmissions = 0; // Кількість негайних передач
  int cycleOffset = 1000;         // Затримка перед передачею (мс)
  int cycleTime = 2000;           // Інтервал між передачами (мс)
  bool networkRequested = false;

  std::mutex mtx;

public:
  void Init()
  {
    std::cout << "[Init] Network Management initialized.\n";
  }

  void EnterRepeatMessageState()
  {
    std::lock_guard<std::mutex> lock(mtx);
    state = NetworkState::REPEAT_MESSAGE;

    std::cout << "[State] Entered Repeat Message State.\n";

    // Реалізація специфікації [SWS_UdpNm_00005]
    if (!networkRequested && immediateTransmissions == 0)
    {
      std::cout << "[Info] No immediate transmission requested, delaying by "
                << cycleOffset << "ms.\n";
      std::this_thread::sleep_for(std::chrono::milliseconds(cycleOffset));
    }

    SendNmPdu(); // Після затримки відправляємо повідомлення
  }

  void StartCycleTimer()
  {
    // Реалізація специфікації [SWS_UdpNm_00040]
    std::thread([this]()
                {
            while (!stopTimers) {
                std::this_thread::sleep_for(std::chrono::milliseconds(cycleTime));
                if (state == NetworkState::OPERATIONAL || state == NetworkState::REPEAT_MESSAGE) {
                    std::cout << "[Timer] Cycle expired. Sending NM PDU.\n";
                    SendNmPdu();
                }
            } })
        .detach();
  }

  void NetworkRequest()
  {
    std::lock_guard<std::mutex> lock(mtx);
    std::cout << "[Request] Network requested.\n";
    networkRequested = true;
    state = NetworkState::OPERATIONAL;
    SendNmPdu();
  }

  void NetworkRelease()
  {
    // Реалізація специфікації [SWS_UdpNm_00105]
    std::lock_guard<std::mutex> lock(mtx);
    state = NetworkState::RELEASED;
    networkRequested = false;
    std::cout << "[Release] Network released. Resources are freed.\n";
  }

  void SendNmPdu()
  {
    std::cout << "[Send] NM PDU sent. State: " << GetStateName() << "\n";
  }

  std::string GetStateName() const
  {
    switch (state.load())
    {
    case NetworkState::RELEASED:
      return "RELEASED";
    case NetworkState::REPEAT_MESSAGE:
      return "REPEAT_MESSAGE";
    case NetworkState::OPERATIONAL:
      return "OPERATIONAL";
    default:
      return "UNKNOWN";
    }
  }

  void Stop()
  {
    stopTimers = true;
  }
};

// Основна функція програми
int main()
{
  UdpNm udpNm;

  udpNm.Init();            // Ініціалізація менеджменту
  udpNm.StartCycleTimer(); // Запуск циклічного таймера

  udpNm.EnterRepeatMessageState(); // Вхід у стан повторної передачі

  std::this_thread::sleep_for(std::chrono::milliseconds(5000)); // Симуляція часу
  udpNm.NetworkRequest();                                       // Симуляція запиту на мережу

  std::this_thread::sleep_for(std::chrono::milliseconds(5000)); // Ще пауза
  udpNm.NetworkRelease();                                       // Вивільнення мережі

  udpNm.Stop();                                                 // Зупинка таймера перед завершенням
  std::this_thread::sleep_for(std::chrono::milliseconds(1000)); // Дати часу на завершення потоків

  return 0;
}
