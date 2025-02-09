#ifndef MAINWINDOW_HPP
#define MAINWINDOW_HPP

#include <QMainWindow>

namespace Ffmpeg {
class AVError;
}

class MainWindow : public QMainWindow
{
    Q_OBJECT
public:
    MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

private slots:
    void onError(const Ffmpeg::AVError &avError);
    void onVideoEncoderChanged();
    void onOpenInputFile();
    void onCheckInputFile();
    void onOpenSubtitle();
    void onOpenOutputFile();
    void onStart();

private:
    void setupUI();
    void buildConnect();

    class MainWindowPrivate;
    QScopedPointer<MainWindowPrivate> d_ptr;
};

#endif // MAINWINDOW_HPP
