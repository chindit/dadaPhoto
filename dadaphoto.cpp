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

dadaPhoto::~dadaPhoto(){
    delete ui;
    if(!fichiers.empty())
        delete image;
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
    /*for(int i=0; i<fichiers.size(); i++){
        QListWidgetItem *item = new QListWidgetItem(fichiers.at(i));
        ui->listWidget->addItem(item);
        if(i==0){ui->listWidget->setCurrentItem(item);}
    }*/
    //ui->listWidget->setCurrentRow(1, QItemSelectionModel::Select);
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
