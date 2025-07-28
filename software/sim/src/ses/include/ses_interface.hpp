#pragma once

#include <cstdint>
#include <vector>
#include <string>

namespace ses {

/// Структура входного сообщения (команда от CVM к модулю)
struct Command {
    uint8_t command_id;              // Код команды
    std::vector<uint8_t> payload;    // Данные команды
};

/// Структура выходного сообщения (ответ от модуля к CVM)
struct Response {
    uint8_t status_code;             // Код статуса
    std::vector<uint8_t> data;       // Ответные данные
};

/// Интерфейс взаимодействия модуля с CVM
class ModuleInterface {
public:
    virtual ~ModuleInterface() = default;

    /// Метод вызывается CVM, чтобы передать команду модулю
    virtual void receiveCommand(const Command& cmd) = 0;

    /// Метод вызывается CVM, чтобы запросить ответное сообщение от модуля
    virtual Response getResponse() = 0;

    /// Возвращает имя модуля для отладки
    virtual std::string moduleName() const = 0;
};

} // namespace ses