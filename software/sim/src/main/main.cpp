#include "imodule.hpp"
#include "mock_bus.hpp" 
#include "cvs.hpp"
#include "ses.hpp"

// Для демонстрации включаем минимальный DS модуль
#define DS_IMPLEMENTATION
#include "ds.hpp"

#include <iostream>
#include <thread>
#include <chrono>
#include <signal.h>
#include <atomic>

// Глобальный флаг для graceful shutdown
std::atomic<bool> g_shutdown_requested(false);

// Обработчик сигнала прерывания
void signalHandler(int signal) {
    if (signal == SIGINT) {
        std::cout << "\n[Main] Interrupt signal received, initiating shutdown..." << std::endl;
        g_shutdown_requested = true;
    }
}

// Функция для вывода заголовка симулятора
void printHeader() {
    std::cout << "\n";
    std::cout << "╔═══════════════════════════════════════════════════════════════╗\n";
    std::cout << "║         ARACHNOPOD ROBOT SIMULATOR v2.0                      ║\n";
    std::cout << "║         Симулятор робота-паука с UART протоколом             ║\n";
    std::cout << "╠═══════════════════════════════════════════════════════════════╣\n";
    std::cout << "║ Протокол: UART-подобный с 16-битными заголовками             ║\n";
    std::cout << "║ Архитектура: Модульная с центральной шиной данных            ║\n";
    std::cout << "║ Модули: CVS (ЦВС), SES (СЭС), DS (СОДФ)                     ║\n";
    std::cout << "║ Частоты: CVS-50Hz, SES-20Hz, DS-100Hz, Bus-1000Hz           ║\n";
    std::cout << "╚═══════════════════════════════════════════════════════════════╝\n";
    std::cout << "\n";
}

// Функция для вывода инструкций
void printInstructions() {
    std::cout << "Инструкции:\n";
    std::cout << "  - Симуляция запущена в автоматическом режиме\n";
    std::cout << "  - CVS автоматически опрашивает модули и управляет системой\n";
    std::cout << "  - SES управляет питанием и мониторит состояние батареи\n";
    std::cout << "  - DS симулирует систему движения с 18 сервоприводами\n";
    std::cout << "  - Нажмите Ctrl+C для остановки симуляции\n";
    std::cout << "\n";
}

int main() {
    // Установка обработчика сигналов
    signal(SIGINT, signalHandler);
    
    // Вывод заголовка
    printHeader();
    printInstructions();
    
    try {
        // ===== ФАЗА 1: Создание инфраструктуры =====
        std::cout << "┌─────────────────────────────────────────┐\n";
        std::cout << "│ ФАЗА 1: Создание инфраструктуры        │\n";
        std::cout << "└─────────────────────────────────────────┘\n";
        
        // Создание коммуникационной шины
        auto bus = std::make_shared<MockBus>();
        std::cout << "[Main] Коммуникационная шина создана\n";
        
        // ===== ФАЗА 2: Создание модулей =====
        std::cout << "\n┌─────────────────────────────────────────┐\n";
        std::cout << "│ ФАЗА 2: Создание системных модулей     │\n";
        std::cout << "└─────────────────────────────────────────┘\n";
        
        // Создание центральной вычислительной системы
        auto cvs = std::make_shared<CVS>();
        std::cout << "[Main] CVS (Центральная вычислительная система) создана\n";
        
        // Создание системы энергоснабжения
        auto ses = std::make_shared<PowerSupplySystem>();
        std::cout << "[Main] SES (Система энергоснабжения) создана\n";
        
        // Создание системы движения
        auto ds = std::make_shared<DriveSystem>();
        std::cout << "[Main] DS (Система движения) создана\n";
        
        // ===== ФАЗА 3: Инициализация модулей =====
        std::cout << "\n┌─────────────────────────────────────────┐\n";
        std::cout << "│ ФАЗА 3: Инициализация модулей          │\n";
        std::cout << "└─────────────────────────────────────────┘\n";
        
        if (!cvs->initialize(bus)) {
            std::cerr << "[Main] ОШИБКА: Не удалось инициализировать CVS\n";
            return -1;
        }
        std::cout << "[Main] ✓ CVS инициализирована\n";
        
        if (!ses->initialize(bus)) {
            std::cerr << "[Main] ОШИБКА: Не удалось инициализировать SES\n";
            return -1;
        }
        std::cout << "[Main] ✓ SES инициализирована\n";
        
        if (!ds->initialize(bus)) {
            std::cerr << "[Main] ОШИБКА: Не удалось инициализировать DS\n";
            return -1;
        }
        std::cout << "[Main] ✓ DS инициализирована\n";
        
        // ===== ФАЗА 4: Запуск модулей =====
        std::cout << "\n┌─────────────────────────────────────────┐\n";
        std::cout << "│ ФАЗА 4: Запуск модулей                 │\n";
        std::cout << "└─────────────────────────────────────────┘\n";
        
        if (!cvs->start()) {
            std::cerr << "[Main] ОШИБКА: Не удалось запустить CVS\n";
            return -1;
        }
        std::cout << "[Main] ✓ CVS запущена (частота: " << Timing::CVS_FREQUENCY_HZ << " Hz)\n";
        
        if (!ses->start()) {
            std::cerr << "[Main] ОШИБКА: Не удалось запустить SES\n";
            return -1;
        }
        std::cout << "[Main] ✓ SES запущена (частота: " << Timing::SES_FREQUENCY_HZ << " Hz)\n";
        
        if (!ds->start()) {
            std::cerr << "[Main] ОШИБКА: Не удалось запустить DS\n";
            return -1;
        }
        std::cout << "[Main] ✓ DS запущена (частота: " << Timing::DS_FREQUENCY_HZ << " Hz)\n";
        
        std::cout << "\n[Main] Все модули успешно запущены!\n";
        std::cout << "[Main] Шина работает на частоте " << Timing::BUS_FREQUENCY_HZ << " Hz\n";
        
        // ===== ФАЗА 5: Основной цикл симуляции =====
        std::cout << "\n┌─────────────────────────────────────────┐\n";
        std::cout << "│ ФАЗА 5: Симуляция запущена             │\n";
        std::cout << "└─────────────────────────────────────────┘\n";
        std::cout << "[Main] Нажмите Ctrl+C для остановки\n\n";
        
        // Параметры симуляции
        const int SIMULATION_DURATION_SECONDS = 120; // 2 минуты максимум
        const double MAIN_LOOP_FREQUENCY = 10.0; // Главный цикл на 10 Hz (для мониторинга)
        Timing::PrecisionTimer main_timer(MAIN_LOOP_FREQUENCY);
        
        // Счетчики для статистики
        uint32_t seconds_elapsed = 0;
        uint32_t cycles_count = 0;
        
        // Демонстрационные команды для DS
        auto sendDemoCommand = [&](uint32_t command_bits, const std::string& description) {
            std::cout << "\n[Main] >>> Отправка команды DS: " << description << "\n";
            Protocol::DataBlock cmd_block;
            cmd_block.raw = command_bits;
            auto command_msg = Protocol::createCommandRequest(Protocol::ModuleID::DS, {cmd_block});
            bus->sendMessage(command_msg);
        };
        
        main_timer.reset();
        
        while (!g_shutdown_requested && seconds_elapsed < SIMULATION_DURATION_SECONDS) {
            // Выполняем 10 циклов в секунду
            for (int cycle = 0; cycle < (int)MAIN_LOOP_FREQUENCY && !g_shutdown_requested; ++cycle) {
                cycles_count++;
                
                // Демонстрационные команды в определенные моменты времени
                if (seconds_elapsed == 3 && cycle == 0) {
                    // Через 3 секунды - включаем DS через SES
                    std::cout << "\n[Main] >>> Включение системы движения через SES\n";
                    ds->setPowered(true);
                }
                
                if (seconds_elapsed == 5 && cycle == 0) {
                    // Через 5 секунд - команда встать
                    sendDemoCommand(0x10, "Встать (Stand up)");
                }
                
                if (seconds_elapsed == 8 && cycle == 0) {
                    // Через 8 секунд - начать движение
                    sendDemoCommand(0x01, "Начать движение (Start walking)");
                }
                
                if (seconds_elapsed == 15 && cycle == 0) {
                    // Через 15 секунд - повернуть влево
                    sendDemoCommand(0x04, "Повернуть влево (Turn left)");
                }
                
                if (seconds_elapsed == 18 && cycle == 0) {
                    // Через 18 секунд - повернуть вправо
                    sendDemoCommand(0x08, "Повернуть вправо (Turn right)");
                }
                
                if (seconds_elapsed == 22 && cycle == 0) {
                    // Через 22 секунды - остановиться
                    sendDemoCommand(0x02, "Остановиться (Stop)");
                }
                
                if (seconds_elapsed == 25 && cycle == 0) {
                    // Через 25 секунд - калибровка
                    sendDemoCommand(0x40, "Калибровка (Calibrate)");
                }
                
                // Ждем следующий цикл
                main_timer.waitForNextCycle();
                
                // Проверка перегрузки главного цикла
                if (main_timer.isOverrun()) {
                    std::cout << "[Main] ПРЕДУПРЕЖДЕНИЕ: Перегрузка главного цикла\n";
                }
            }
            
            seconds_elapsed++;
            
            // Вывод прогресса каждые 5 секунд
            if (seconds_elapsed % 5 == 0) {
                std::cout << "\n";
                std::cout << "╔════════════════════════════════════════╗\n";
                std::cout << "║ Прогресс симуляции: " << std::setw(3) << seconds_elapsed 
                          << "/" << SIMULATION_DURATION_SECONDS << " секунд      ║\n";
                std::cout << "║ Циклов выполнено: " << std::setw(8) << cycles_count 
                          << "            ║\n";
                std::cout << "╚════════════════════════════════════════╝\n";
            }
            
            // Периодический вывод краткой статистики
            if (seconds_elapsed % 10 == 0 && seconds_elapsed > 0) {
                std::cout << "\n--- Краткая статистика (t=" << seconds_elapsed << "s) ---\n";
                bus->printStatistics();
            }
        }
        
        if (g_shutdown_requested) {
            std::cout << "\n[Main] Получен сигнал остановки от пользователя\n";
        } else {
            std::cout << "\n[Main] Время симуляции истекло\n";
        }
        
        // ===== ФАЗА 6: Завершение работы =====
        std::cout << "\n┌─────────────────────────────────────────┐\n";
        std::cout << "│ ФАЗА 6: Завершение работы              │\n";
        std::cout << "└─────────────────────────────────────────┘\n";
        
        // Остановка модулей (это остановит их рабочие потоки)
        std::cout << "[Main] Остановка модулей...\n";
        cvs->stop();
        std::cout << "[Main] ✓ CVS остановлена\n";
        
        ses->stop();
        std::cout << "[Main] ✓ SES остановлена\n";
        
        ds->stop();
        std::cout << "[Main] ✓ DS остановлена\n";
        
        // Остановка шины
        std::cout << "[Main] Остановка коммуникационной шины...\n";
        bus->shutdown();
        std::cout << "[Main] ✓ Шина остановлена\n";
        
        // ===== ФАЗА 7: Финальная статистика =====
        std::cout << "\n┌─────────────────────────────────────────┐\n";
        std::cout << "│ ФАЗА 7: Финальная статистика           │\n";
        std::cout << "└─────────────────────────────────────────┘\n";
        
        bus->printStatistics();
        
        std::cout << "\n[Main] Дополнительная статистика:\n";
        std::cout << "  - Время работы: " << seconds_elapsed << " секунд\n";
        std::cout << "  - Циклов главного потока: " << cycles_count << "\n";
        std::cout << "  - Средняя частота: " << (double)cycles_count/seconds_elapsed << " Hz\n";
        
        std::cout << "\n╔═══════════════════════════════════════════════════════════════╗\n";
        std::cout << "║         СИМУЛЯЦИЯ ЗАВЕРШЕНА УСПЕШНО                          ║\n";
        std::cout << "╚═══════════════════════════════════════════════════════════════╝\n\n";
        
        return 0;
        
    } catch (const std::exception& e) {
        std::cerr << "\n[Main] КРИТИЧЕСКАЯ ОШИБКА: " << e.what() << "\n";
        return -1;
    } catch (...) {
        std::cerr << "\n[Main] НЕИЗВЕСТНАЯ КРИТИЧЕСКАЯ ОШИБКА\n";
        return -1;
    }
}