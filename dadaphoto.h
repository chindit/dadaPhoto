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
#include <QPlainTextEdit>
#include <QProcess>
#include <QSize>
#include <QString>
#include <QTransform>

#include <exiv2/exiv2.hpp>
#include <iostream>
#include <iomanip>
#include <cassert>

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
    void readExif(QString nom);

    Ui::dadaPhoto *ui;
    QLabel *imageLabel;
    QDir dossier;
    QPixmap *image;
    QStringList supprimer,imprimer,visitees,fichiers;
    QString currentPhoto;
    QPlainTextEdit *copyOutput;
    QProcess *gphoto2;
    QLabel *actuel;

    bool changement;
    int nbPhotos, position;

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
    void importPictures();
    void readyReadStandardOutput();
    void cleanDirectory();

private:
    void setPictureList();

};

#endif // DADAPHOTO_H
