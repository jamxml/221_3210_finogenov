#include "mainwindow.h"
#include "ui_mainwindow.h"

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent), ui(new Ui::MainWindow), moveCounter(0) {
    ui->setupUi(this);

    // Если в ui нет макета для основного окна, создаем новый
    QVBoxLayout *mainLayout = new QVBoxLayout();  // Создаем вертикальный макет для основного окна

    // Настройка 4x4 поля чекбоксов
    for (int i = 0; i < 4; ++i) {
        for (int j = 0; j < 4; ++j) {
            QCheckBox *checkBox = new QCheckBox(this);
            checkBox->setObjectName(QString("checkbox_%1_%2").arg(i).arg(j));
            checkBox->setFixedSize(50, 50);
            ui->gridLayout_2->addWidget(checkBox, i, j);
            connect(checkBox, &QCheckBox::stateChanged, this, &MainWindow::handleCheckboxClick);
        }
    }

    // Создаем вертикальный макет для кнопок и размещаем их внизу
    QVBoxLayout *buttonLayout = new QVBoxLayout();
    buttonLayout->addWidget(ui->loadButton);
    buttonLayout->addWidget(ui->resetButton);

    // Создаем отдельный виджет для кнопок и добавляем в макет
    QWidget *buttonWidget = new QWidget(this);
    buttonWidget->setLayout(buttonLayout);

    // Добавляем этот виджет в основной макет
    mainLayout->addWidget(ui->gamePage);
    mainLayout->addWidget(buttonWidget);

    // Устанавливаем основной макет для главного окна
    QWidget *centralWidget = new QWidget(this);
    centralWidget->setLayout(mainLayout);
    setCentralWidget(centralWidget);  // Назначаем центральный виджет с нашим макетом

    // Подключаем обработчики событий кнопок
    connect(ui->loadButton, &QPushButton::clicked, this, &MainWindow::loadGameFromTxt);
    connect(ui->resetButton, &QPushButton::clicked, this, &MainWindow::resetGame);
}

MainWindow::~MainWindow() {
    delete ui;
}

// Обработка нажатия на чекбокс
void MainWindow::handleCheckboxClick(int state) {
    if (state != Qt::Checked) return;

    QCheckBox *checkBox = qobject_cast<QCheckBox*>(sender());
    if (!checkBox) return;

    QString color = (moveCounter % 2 == 0) ? "green" : "red";
    checkBox->setStyleSheet(QString("background-color: %1;").arg(color));
    checkBox->setEnabled(false);
    moveCounter++;

    int x = checkBox->objectName().split('_')[1].toInt();
    int y = checkBox->objectName().split('_')[2].toInt();
    QString timestamp = QDateTime::currentDateTime().toString("yyyy.MM.dd_HH:mm:ss");
    QString checksum = generateChecksum(x, y, color, timestamp);

    QJsonObject move;
    move["x"] = x;
    move["y"] = y;
    move["color"] = color;
    move["timestamp"] = timestamp;
    move["checksum"] = checksum;

    // Запись хода в текстовый файл
    saveGameToTextFile(move);
}

// Генерация контрольной суммы
QString MainWindow::generateChecksum(int x, int y, const QString &color, const QString &timestamp) {
    QByteArray data = QString("%1%2%3%4").arg(x).arg(y).arg(color).arg(timestamp).toUtf8();
    return QString(QCryptographicHash::hash(data, QCryptographicHash::Sha256).toHex());
}

// Сохранение состояния в JSON
void MainWindow::saveGameToTextFile(const QJsonObject &move) {
    QFile file("gameState.txt");

    // Открываем файл для добавления данных в конец
    if (!file.open(QIODevice::Append | QIODevice::Text)) {
        qWarning() << "Не удалось открыть файл для записи!";
        return;
    }

    QTextStream out(&file);

    // Извлекаем данные из объекта хода
    int x = move["x"].toInt();
    int y = move["y"].toInt();
    QString color = move["color"].toString();
    QString timestamp = move["timestamp"].toString();
    QString checksum = move["checksum"].toString();

    // Формат записи в файл: каждая строка — отдельное поле данных
    out << "x: " << x << "\n";
    out << "y: " << y << "\n";
    out << "color: " << color << "\n";
    out << "timestamp: " << timestamp << "\n";
    out << "checksum: " << checksum << "\n";
    out << "-----------------------\n";  // Разделитель между ходами

    file.close();
}


// Загрузка состояния из JSON
void MainWindow::loadGameFromTxt() {
    QFile file("gameState.txt");
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        qWarning() << "Не удалось открыть файл для чтения!";
        return;
    }

    QTextStream in(&file);
    QString line;
    int moveIndex = 0;

    while (!in.atEnd()) {
        line = in.readLine();
        if (line.startsWith("x: ")) {
            int x = line.mid(3).toInt();
            line = in.readLine();
            int y = line.mid(3).toInt();
            line = in.readLine();
            QString color = line.mid(7);
            line = in.readLine();
            QString timestamp = line.mid(11);
            line = in.readLine();
            QString checksum = line.mid(10);

            // Валидация контрольной суммы
            QString computedChecksum = generateChecksum(x, y, color, timestamp);
            if (checksum != computedChecksum) {
                QMessageBox::warning(this, "Ошибка контрольной суммы", QString("Ошибка в ходе %1").arg(++moveIndex));
                continue;
            }

            // Найдем соответствующий чекбокс и обновим его состояние
            QCheckBox *checkBox = findChild<QCheckBox*>(QString("checkbox_%1_%2").arg(x).arg(y));
            if (checkBox) {
                checkBox->setEnabled(false);
                checkBox->setStyleSheet(QString("background-color: %1;").arg(color));
                checkBox->setChecked(true);
            }
        }

        // Пропускаем строку с разделителем
        in.readLine();
    }

    file.close();
}

// Проверка контрольной суммы
void MainWindow::validateChecksum(const QJsonObject &move, int moveIndex) {
    int x = move["x"].toInt();
    int y = move["y"].toInt();
    QString color = move["color"].toString();
    QString timestamp = move["timestamp"].toString();
    QString savedChecksum = move["checksum"].toString();

    QString computedChecksum = generateChecksum(x, y, color, timestamp);
    if (savedChecksum != computedChecksum) {
        QMessageBox::warning(this, "Ошибка контрольной суммы", QString("Ошибка в ходе %1").arg(moveIndex));
    }
}

// Сброс игрового поля
void MainWindow::resetGame() {
    // Сбрасываем состояние игрового поля
    for (int i = 0; i < 4; ++i) {
        for (int j = 0; j < 4; ++j) {
            QCheckBox *checkBox = findChild<QCheckBox*>(QString("checkbox_%1_%2").arg(i).arg(j));
            if (checkBox) {
                checkBox->setEnabled(true);
                checkBox->setStyleSheet("");
                checkBox->setChecked(false);
            }
        }
    }

    // Перезаписываем состояние в файл (пустое состояние после сброса)
    QFile file("gameState.txt");
    if (file.open(QIODevice::WriteOnly | QIODevice::Text)) {
        file.close();
    }

    moveCounter = 0;  // Сброс счетчика ходов
}
