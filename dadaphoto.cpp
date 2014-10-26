#include "dadaphoto.h"
#include "ui_dadaphoto.h"

dadaPhoto::dadaPhoto(QWidget *parent) : QMainWindow(parent), ui(new Ui::dadaPhoto){
    ui->setupUi(this);
    ui->verticalLayout->setSizeConstraint(QLayout::SetMaximumSize);

    changement = false;

    //Préparation du menu
    connect(ui->action_propos_de_Qt, SIGNAL(triggered()), qApp, SLOT(aboutQt()));
    connect(ui->action_propos, SIGNAL(triggered()), this, SLOT(about()));
    connect(ui->actionOuvrir_un_dossier, SIGNAL(triggered()), this, SLOT(chooseDir()));
    connect(ui->actionQuitter, SIGNAL(triggered()), this, SLOT(askQuit()));
    connect(ui->actionSauvegarder, SIGNAL(triggered()), this, SLOT(prepareApply()));
    connect(ui->actionImporter, SIGNAL(triggered()), this, SLOT(importPictures()));

    //Préparation des boutons
    ui->pushButton_imprimer->setCheckable(true);
    ui->pushButton_supprimer->setCheckable(true);
    connect(ui->pushButton_imprimer, SIGNAL(clicked(bool)), this, SLOT(verifieEtat(bool)));
    connect(ui->pushButton_supprimer, SIGNAL(clicked(bool)), this, SLOT(verifieEtat(bool)));
    connect(ui->pushButton_valider, SIGNAL(clicked()), this, SLOT(prepareApply()));
    connect(ui->pushButton_suivant, SIGNAL(clicked()), this, SLOT(nextImage()));
    connect(ui->pushButton_precedent, SIGNAL(clicked()), this, SLOT(previousImage()));
    connect(ui->pushButton_antihoraire, SIGNAL(clicked()), this, SLOT(rotateLeft()));
    connect(ui->pushButton_horaire, SIGNAL(clicked()), this, SLOT(rotateRight()));

    //Par défaut, c'est ~/Images/Photos qu'on charge
    dossier = QDir::homePath()+"/Images/Photos";
    QDir dirImages(dossier);
    if(!dirImages.exists()){
        int resultat = QMessageBox::question(this, "Dossier non trouvé", "Le dossier «Images/Photos» n'existe pas.\nVeux-tu charger les images d'un autre dossier?\nSi tu réponds «Non», le programme restera vide", QMessageBox::Yes|QMessageBox::No);
        if(resultat == QMessageBox::Yes){
            QString chemin = QFileDialog::getExistingDirectory(this);
            if(!chemin.isEmpty() && !chemin.isNull()){
                dossier.setPath(chemin);
            }
        }
    }
    //Si on est ici, c'est que le dossier est prêt
    //On charge la liste des fichiers dans le QListWidget
    this->setPictureList();
}

dadaPhoto::~dadaPhoto(){
    delete ui;
    if(!fichiers.empty())
        delete image;
}

void dadaPhoto::setPictureList(){
    dossier.setNameFilters(QStringList()<<"*.jpg"<<"*.JPG"<<"*.png"<<"*.PNG"<<"*.jpeg"<<"*.JPEG"<<"*.raw"<<"*.RAW"<<"*.gif"<<"*.GIF"<<"*.bmp"<<"*.BMP");
    fichiers = dossier.entryList(QDir::Files);
    ui->listWidget->addItems(fichiers);

    connect(ui->listWidget, SIGNAL(itemSelectionChanged()), this, SLOT(changePhoto()));

    if(!fichiers.empty()){
        image = new QPixmap(dossier.path()+"/"+fichiers.first());
        this->readExif(dossier.path()+"/"+fichiers.first());
        currentPhoto = fichiers.first();
        ui->label_nombre->setText("1/"+QString::number(fichiers.size()));
    }
}

void dadaPhoto::resizeEvent(QResizeEvent* event)
{
   QMainWindow::resizeEvent(event);
   if(!fichiers.empty()){
       this->redimensionne();
   }
}

void dadaPhoto::redimensionne(){
    QPixmap imageResized;
    QSize taille = image->size();
    //Vérification de la taille et mise à l'échelle
    QSize espaceDispo = ui->zoneImage->size();
    float facteur = 1;
    if(espaceDispo.width() < taille.width() || espaceDispo.height() < taille.height()){
        //On prend le plus grand resultat
        float hauteur = (float)taille.height() / (float)espaceDispo.height();
        float largeur = (float)taille.width() / (float)espaceDispo.width();
        facteur = (largeur < hauteur) ? hauteur : largeur;
    }
    imageResized = image->scaled(((int)taille.width()/facteur), ((int)taille.height()/facteur));
    ui->zoneImage->setPixmap(imageResized);
}

void dadaPhoto::changePhoto(int pos){
    QString nom;
    int posValide = 0;
    if(pos == -2){
        //Activé si on clique sur un nom
        nom = dossier.path()+"/"+ui->listWidget->currentItem()->text();
        posValide = fichiers.indexOf(ui->listWidget->currentItem()->text());
    }
    else if(pos == -1){
        nom = dossier.path()+"/"+fichiers.last();
        posValide = (fichiers.size()-1);
        ui->listWidget->setCurrentRow(ui->listWidget->count()-1);
    }
    else{
        if(pos >= fichiers.size()){
            nom = dossier.path()+"/"+fichiers.first();
            posValide = 0;
            ui->listWidget->setCurrentRow(0);
        }
        else{
            nom = dossier.path()+"/"+fichiers.at(pos);
            posValide = pos;
            ui->listWidget->setCurrentRow(posValide);
        }
    }
    delete image;
    image = new QPixmap(nom);
    this->readExif(nom);
    currentPhoto = fichiers.at(posValide);

    //Actualisation de la position
    ui->label_nombre->setText(QString::number(posValide+1)+"/"+QString::number(fichiers.size()));

    //Redimensionnement
    this->redimensionne();

    //Ajout à la liste des images déjà visitée
    if(!visitees.contains(fichiers.at(posValide))){
        visitees.append(fichiers.at(posValide));
    }

    //On met à jour les boutons
    if(imprimer.contains(fichiers.at(posValide))){
        ui->pushButton_imprimer->setChecked(true);
    }
    else{
        ui->pushButton_imprimer->setChecked(false);
    }
    if(supprimer.contains(fichiers.at(posValide))){
        ui->pushButton_supprimer->setChecked(true);
    }
    else{
        ui->pushButton_supprimer->setChecked(false);
    }
}

void dadaPhoto::verifieEtat(bool etat){
    //Pour sauvegarde
    if(!changement){
        changement = true;
    }

    //Alerte de sécurité
    if(ui->pushButton_imprimer->isChecked() && ui->pushButton_supprimer->isChecked()){
        QMessageBox::information(this, "Double acction", "ATTENTION!  Tu as sélectionné À LA FOIS l'action de supprime ET d'imprimer la photo.\nLes deux n'étant pas possible en même temps, il faut choisir :-)");
    }

    if(etat){
        //Ajout à la QStringList
        if((ui->pushButton_imprimer->isChecked() && !ui->pushButton_supprimer->isChecked()) && !imprimer.contains(currentPhoto)){
            imprimer.append(currentPhoto);
        }
        if((!ui->pushButton_imprimer->isChecked() && ui->pushButton_supprimer->isChecked()) && !supprimer.contains(currentPhoto)){
            supprimer.append(currentPhoto);
        }
    }
    else{
        //Suppression de la QStringList
        if(!ui->pushButton_imprimer->isChecked() && imprimer.contains(currentPhoto)){
            imprimer.removeAt(imprimer.indexOf(currentPhoto));
        }
        if(!ui->pushButton_supprimer->isChecked() && supprimer.contains(currentPhoto)){
            supprimer.removeAt(supprimer.indexOf(currentPhoto));
        }
    }
}

void dadaPhoto::prepareApply(){
    //On affiche les changements
    if(imprimer.size()+supprimer.size() < 35){
        QString message = "<center><h1>Changements</h1></center>";
        message += "<h3>Impression:</h3>";
        for(int i=0; i<imprimer.size(); i++){
            message+= imprimer.at(i)+"<br/>";
        }
        message += "<br><h3>Suppression</h3>";
        for(int i=0; i<supprimer.size(); i++){
            message+= supprimer.at(i)+"<br>";
        }
        QMessageBox::information(this, "Changements", message);
    }
    this->apply();
    changement = false;
}

void dadaPhoto::apply(){
    //1 créer un nouveau dossier
    //QDate current = QDate::currentDate();
    //QString nomDossier = current.toString("ddMMyy");
    QDir dirDossier =  QDir(QDir::homePath()+"/Images/Photos_vues");
    if(!dirDossier.exists()){
        //Le dossier n'existe pas, on peut le créer
        dirDossier.mkdir(dirDossier.path());
    }
    //Création des autres dossiers
    dirDossier.setPath(QDir::homePath()+"/Images/Imprimer");
    if(!dirDossier.exists()){
        dirDossier.mkdir(dirDossier.path());
    }

    //2 Supprimer
    for(int i=0; i<supprimer.size(); i++){
        dirDossier.remove(dossier.path()+"/"+supprimer.at(i));
    }
    supprimer.clear();

    //3 Copier
    for(int i=0; i<imprimer.size(); i++){
        QFile::copy(dossier.path()+"/"+imprimer.at(i), QDir::homePath()+"/Images/Imprimer/"+imprimer.at(i));
    }
    imprimer.clear();

    //4 Déplacer
    for(int i=0; i<visitees.size(); i++){
        QFile photo; photo.setFileName(dossier.path()+"/"+visitees.at(i));
        if(photo.exists()){
            photo.rename(QDir::homePath()+"/Images/Photos_vues/"+visitees.at(i));
        }
    }
    visitees.clear();

    //Petit message
    QMessageBox::information(this, "Fini!", "Voilà, tout a été déplacé et/ou supprimé.  Pour rappel, les photos à imprimer se trouvent ici:"+QDir::homePath()+"/Images/Imprimer et les photos visitées ici: "+QDir::homePath()+"/Images/Photos_vues");

    //Et on ré-actualise la liste
    ui->listWidget->clear();
    this->setPictureList();
}

void dadaPhoto::nextImage(){
    int pos = fichiers.indexOf(currentPhoto);
    this->changePhoto(pos+1);
}

void dadaPhoto::previousImage(){
    int pos = fichiers.indexOf(currentPhoto);
    this->changePhoto(pos-1);
}

void dadaPhoto::about(){
    QMessageBox::about(this, "À propos de dadaPhoto", "<h3>À propos de dadaPhoto</h3><b>Version: </b>0.2.0<br><b>Créé par: </b>David Lumaye");
}

void dadaPhoto::chooseDir(){
    QString chemin = QFileDialog::getExistingDirectory(this);
    if(!chemin.isEmpty() && !chemin.isNull()){
        dossier.setPath(chemin);
    }
    else{
        return;
    }
    dossier.setNameFilters(QStringList()<<"*.jpg"<<"*.JPG"<<"*.png"<<"*.PNG"<<"*.jpeg"<<"*.JPEG"<<"*.raw"<<"*.RAW"<<"*.gif"<<"*.GIF"<<"*.bmp"<<"*.BMP");
    fichiers = dossier.entryList(QDir::Files);

    ui->listWidget->addItems(fichiers);
    if(!fichiers.empty()){
        image = new QPixmap(dossier.path()+"/"+fichiers.first());
        currentPhoto = fichiers.first();
        //Par prudence, on actualise
        this->redimensionne();
    }
    if(fichiers.size() > 0){
        ui->listWidget->setCurrentRow(0);
    }
}

void dadaPhoto::askQuit(){
    if(changement){
        int reponse = QMessageBox::warning(this, "Changements non enregistrés", "<b>ATTENTION</b>: Des modifications n'ont pas été enregistrées.  Veux-tu les appliquer ou non?", QMessageBox::Yes | QMessageBox::No | QMessageBox::Cancel);
        if(reponse == QMessageBox::Cancel){
            return;
        }
        if(reponse == QMessageBox::No){
            qApp->quit();
        }
        if(reponse == QMessageBox::Yes){
            this->prepareApply();
            qApp->quit();
        }
    }
    else{
        this->close();
    }
}

void dadaPhoto::rotateLeft(){
    QTransform rotation;
    rotation.rotate(-90);
    QPixmap temp = image->transformed(rotation);
    delete image;
    image = new QPixmap(temp);
    this->redimensionne();
}

void dadaPhoto::rotateRight(){
    QTransform rotation;
    rotation.rotate(90);
    QPixmap temp = image->transformed(rotation);
    delete image;
    image = new QPixmap(temp);
    this->redimensionne();
}

void dadaPhoto::readExif(QString nom){
    std::string nomImage; nomImage = nom.toStdString();
    Exiv2::Image::AutoPtr image = Exiv2::ImageFactory::open(nomImage);
       assert(image.get() != 0);
       image->readMetadata();

       Exiv2::ExifData &exifData = image->exifData();
       if (exifData.empty()) {
           std::string error(nomImage);
           error += ": No Exif data found in the file";
           throw Exiv2::Error(1, error);
       }
       Exiv2::ExifData::const_iterator end = exifData.end();
       for (Exiv2::ExifData::const_iterator i = exifData.begin(); i != end; ++i) {
           if(i->key() == "Exif.Image.Orientation"){
               if(i->value().toLong() == 6){
                   //Si l'image est en mode portrait, on effectue une rotation de 90° horaire pour qu'elle s'affiche correctement
                   this->rotateRight();
               }
               else{
                   //On ne fait rien, l'image est bien mise
               }
               break;
           }//Fin du «if»
       }//Fin de la bougle
       return;
}

void dadaPhoto::importPictures(){
    gphoto2 = new QProcess;
    gphoto2->start("gphoto2 -v");
    gphoto2->waitForFinished();
    if(gphoto2->error() != QProcess::UnknownError){
        QMessageBox::critical(this, "Erreur fatale", "Une erreur est survenue lors du lancement du programme d'import de photos.\nSoit le programme (gphoto2) n'est pas installé, soit il ne s'appelle plus «gphoto2».\nDans tous les cas, il m'est impossible d'importer les photos.\nDésolé :(");
        return;
    }
    bool continuer=true;
    while(continuer){
        gphoto2->start("bash", QStringList() << "-c" << "gphoto2 --auto-detect | grep -c usb");
        gphoto2->waitForFinished();
        //On passe par QString puis INT pour éviter des problèmes de mauvaise conversion
        QString stringResultat = QString(gphoto2->readAll().trimmed());
        int valeur = stringResultat.toInt();
        if(valeur == 0){
            gphoto2->close();
            int reponse = QMessageBox::warning(this, "Pas d'appareil photo", "L'appareil photo n'a <b>pas pu être trouvé</b><br />Pour résoudre ce problème:<br /> \
                                                               1)Vérifie que le cable est bien branché<br /> \
                    2)Éteins et rallume l'appareil photo [ON->OFF->ON].  Il n'est détectable <b>QUE</b> pendant 10 secondes, ce qui peut expliquer le problème<br /> \
                    3)Si ça ne fonctionne toujours pas, débranche l'appareil photo, retire la carte, remets-la et rebranche tout.<br /> \
                    4)Refais les 3 premières étapes <br /> \
                    5)Si ça échoue toujours, abandonne et appelle quelqu'un", QMessageBox::Retry | QMessageBox::Abort);
            if(reponse == QMessageBox::Abort){
                return;
            }
        }
        else{
            continuer = false;
        }
    }
    //On liste les photos
    gphoto2->close();
    gphoto2->start("bash", QStringList() << "-c" << "gphoto2 --list-files | wc -l");
    gphoto2->waitForFinished();
    bool isOk = false;
    QString conversion = QString(gphoto2->readAll().trimmed());
    nbPhotos = conversion.toInt(&isOk);
    if(!isOk){
        QMessageBox::critical(this, "Impossible de détecter les photos", "<b>HÉLAS…</b> Il n'est pas possible de déterminer le nombre de photos à importer :(<br /> \
                              <b>Que faire?</b><br /> \
                                           1)Éteins et ralumme l'appareil photo.  Il n'est détectable <b>QUE</b> pendant 10 secondes, ce qui pose un <b>GROS</b> problème<br /> \
                2)Clique sur «OK» (dans cette boîte) et recommence à faire «Fichier>Importer» et tout <i>devrait</i> fonctionner.<br /> \
                Et <i>zen</i>, c'est rien de grave.");
                return;
    }
    gphoto2->close();
    //Initialisation de la position
    position = 1;

    QDialog *telechargement = new QDialog;
    telechargement->setWindowIcon(QIcon(":/images/dadaphoto.png"));
    QLabel *titre = new QLabel("Téléchargement des photos");
    actuel = new QLabel("1");
    QLabel *total = new QLabel("/"+QString::number(nbPhotos));
    copyOutput = new QPlainTextEdit();
    QGridLayout *layout = new QGridLayout;
    layout->addWidget(titre, 0, 0, 1, 2, Qt::AlignCenter);
    layout->addWidget(actuel, 1, 0);
    layout->addWidget(total, 1, 1);
    layout->addWidget(copyOutput, 2, 0, 1, 2, Qt::AlignCenter);
    telechargement->setLayout(layout);
    telechargement->show();

    gphoto2->start("/home/leonard/download.sh");
    connect(gphoto2,SIGNAL(readyReadStandardOutput()),this,SLOT(readyReadStandardOutput()));

    //EventLoop pour attendre la fin de la copie.
    QEventLoop eventLoop;
    QObject::connect(gphoto2, SIGNAL(finished(int)), &eventLoop, SLOT(quit()));
    eventLoop.exec();
    telechargement->close();

    QMessageBox::information(this, "Import terminé", "Voilà, toutes les photos ont été importées.  Tu peux maintenant les regarder normalement.");
    /*int reponse = QMessageBox::question(this, "Supprimer les photos de la carte?", "dadaPhotos te permet de supprimer automatiquement toutes les photos de la carte SD (sur ton appareil photo)<br /><b>ATTENTION</b>Cette opération est irréversible. <br />Souhaites-tu supprimer les photos de la carte SD?", QMessageBox::Yes | QMessageBox::No);
    if(reponse == QMessageBox::Yes){
        gphoto2->close();
        gphoto2->start("bash", QStringList() << "-c" << "gphtoto2 --delet-all-files");
        QMessageBox temp;
        temp.setStandardButtons(QMessageBox::Ok);
        temp.setIcon(QMessageBox::Information);
        temp.setText("Les photos sont en cours de suppression.  Ce message se fermera quand l'opération sera terminée");
        temp.setWindowTitle("Suppression en cours");
        temp.show();
        gphoto2->waitForFinished();
        connect(gphoto2, SIGNAL(finished(int)), &temp, SLOT(close()));
    }*/

    //On actualise la liste d'images
    ui->listWidget->clear();
    this->setPictureList();

    delete gphoto2;
    delete copyOutput;
    delete actuel;
}

void dadaPhoto::readyReadStandardOutput(){
    copyOutput->insertPlainText(QString(gphoto2->readAllStandardOutput()));
    position++;
    actuel->setText(QString::number(position));
    QTextCursor c =  copyOutput->textCursor();
    c.movePosition(QTextCursor::End);
    copyOutput->setTextCursor(c);
    copyOutput->ensureCursorVisible();
    return;
}
