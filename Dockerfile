# Не просто создаём образ, но даём ему имя build
FROM gcc:11.3 as build

RUN apt update && \
    apt install -y \
      python3-pip \
      cmake \
    && \
    pip3 install conan==1.59
# Явно указываю версию conan, так как начиная с 2 версии генератор cmake упразднен
# Требуется выбирать другой, например CMakeToolchain. Проще загрузить старый 
# Для новых версий conan потребуется добавить строку RUN conan profile detect

# Запуск conan как раньше
COPY conanfile.txt /app/
RUN mkdir /app/build && cd /app/build && \
    conan install .. --build=missing -s build_type=Release -s compiler.libcxx=libstdc++11

# Добавляем основные исходники
COPY ./src /app/src
# Добавляем файлы тестов
COPY ./tests /app/tests 

COPY CMakeLists.txt /app/

RUN cd /app/build && \
    cmake -DCMAKE_BUILD_TYPE=Release .. && \
    cmake --build .

# Второй контейнер в том же докерфайле
FROM ubuntu:22.04 as run

# Создадим пользователя www
RUN groupadd -r www && useradd -r -g www www
USER www

# Установим переменную окружения GAME_DB_URL
ENV GAME_DB_URL=postgres://postgres:Mys3Cr3t@localhost:30432/

# Скопируем приложение со сборочного контейнера в директорию /app.
# Не забываем также папку data, она пригодится.
COPY --from=build /app/build/bin/game_server /app/
COPY ./data /app/data
# Добавляем статические данные
COPY ./static /app/static 

# Всё что в legacy не используется программой

# Запускаем игровой сервер
ENTRYPOINT ["/app/game_server", "--config-file=/app/data/config.json", "--www-root=/app/static/"]
