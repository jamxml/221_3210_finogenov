#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QCheckBox>
#include <QVBoxLayout>
#include <QGridLayout>
#include <QJsonObject>
#include <QJsonArray>
#include <QJsonDocument>
#include <QFile>
#include <QMessageBox>
#include <QDateTime>
#include <QCryptographicHash>

QT_BEGIN_NAMESPACE
namespace Ui { class MainWindow; }
QT_END_NAMESPACE

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

private slots:
    void handleCheckboxClick(int state);
    void loadGameFromTxt();
    void resetGame();

private:
    Ui::MainWindow *ui;
    int moveCounter;
    void saveGameToTextFile(const QJsonObject &move);
    QString generateChecksum(int x, int y, const QString &color, const QString &timestamp);

    void validateChecksum(const QJsonObject &move, int moveIndex);
};

#endif // MAINWINDOW_H
