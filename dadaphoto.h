#ifndef DADAPHOTO_H
#define DADAPHOTO_H

#include <QDate>
#include <QDir>
#include <QFileDialog>
#include <QImage>
#include <QLabel>
#include <QMainWindow>
#include <QMessageBox>
#include <QPixmap>
#include <QSize>
#include <QString>
#include <QTransform>

namespace Ui {
class dadaPhoto;
}

class dadaPhoto : public QMainWindow
{
    Q_OBJECT

public:
    explicit dadaPhoto(QWidget *parent = 0);
    ~dadaPhoto();

public slots:
    void changePhoto(int pos=-2);
    void resizeEvent(QResizeEvent *);

private:
    void redimensionne();

    Ui::dadaPhoto *ui;
    QLabel *imageLabel;
    QDir dossier;
    QPixmap *image;
    QStringList supprimer,imprimer,visitees,fichiers;
    QString currentPhoto;
    bool changement;

private slots:
    void verifieEtat(bool etat);
    void prepareApply();
    void apply();
    void nextImage();
    void previousImage();
    void about();
    void chooseDir();
    void askQuit();
    void rotateLeft();
    void rotateRight();

};

#endif // DADAPHOTO_H
