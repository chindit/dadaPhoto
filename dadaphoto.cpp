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
    connect(ui->actionNettoyer, SIGNAL(triggered()), this, SLOT(cleanDirectory()));
    connect(ui->actionRefreshList, SIGNAL(triggered()), this, SLOT(setPictureList()));

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
#ifdef Q_OS_LINUX
    // Obsolete.  Should be removed
    dossier.setPath(QDir::homePath()+"/Images/Photos");
    QDir picturesDirectory(dossier);
    if (!picturesDirectory.exists()) {
        dossier = QDir(QStandardPaths::writableLocation(QStandardPaths::PicturesLocation));
    }
#else
    // Should be default
    dossier = QDir(QStandardPaths::writableLocation(QStandardPaths::PicturesLocation));
#endif
    QDir dirImages(dossier);
    if(!dirImages.exists()){
        int resultat = QMessageBox::question(this, "Dossier non trouvé", QString("Le dossier «%1» n'existe pas.\nVeux-tu charger les images d'un autre dossier?\nSi tu réponds «Non», le programme restera vide").arg(this->dossier.absolutePath()), QMessageBox::Yes|QMessageBox::No);
        if(resultat == QMessageBox::Yes){
            QString chemin = QFileDialog::getExistingDirectory(this);
            if(!chemin.isEmpty() && !chemin.isNull()){
                dossier.setPath(chemin);
            }
        }
    }

    //On détecte les doublons
    this->detectDirtyDirectory();

    //Si on est ici, c'est que le dossier est prêt
    //On charge la liste des fichiers dans le QListWidget
    this->setPictureList();

    //Connexion de l'action Quitter
    connect(qApp, SIGNAL(aboutToQuit()), this, SLOT(close()));
}

dadaPhoto::~dadaPhoto(){
    delete ui;
    if(!fichiers.empty())
        delete image;
}

void dadaPhoto::setPictureList(){
    dossier.setNameFilters(QStringList()<<"*.jpg"<<"*.JPG"<<"*.png"<<"*.PNG"<<"*.jpeg"<<"*.JPEG"<<"*.raw"<<"*.RAW"<<"*.gif"<<"*.GIF"<<"*.bmp"<<"*.BMP");
    fichiers = dossier.entryList(QDir::Files);
    ui->listWidget->clear();
    ui->listWidget->addItems(fichiers);

    connect(ui->listWidget, SIGNAL(itemSelectionChanged()), this, SLOT(changePhoto()));

    if(!fichiers.empty()){
        image = new QPixmap(dossier.path()+"/"+fichiers.first());
        this->readExif(dossier.path()+"/"+fichiers.first());
        currentPhoto = fichiers.first();
        ui->label_nombre->setText("1/"+QString::number(fichiers.size()));
        ui->listWidget->setCurrentRow(0);

        // Toggle buttons
        ui->pushButton_antihoraire->setEnabled(true);
        ui->pushButton_horaire->setEnabled(true);
        ui->pushButton_imprimer->setEnabled(true);
        ui->pushButton_precedent->setEnabled(true);
        ui->pushButton_suivant->setEnabled(true);
        ui->pushButton_supprimer->setEnabled(true);
        ui->pushButton_valider->setEnabled(true);
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

    //On vérifie si l'image existe.  Si non, on quitte (survient lors de l'action «Valider» et du renouvellement de QListWidget
    QFile photo; photo.setFileName(nom);
    if(!photo.exists()){
        return;
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
        ui->pushButton_imprimer->setChecked(false);
        ui->pushButton_supprimer->setChecked(false);
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
    this->prepareDestinationDirectories();

    //1) Deletion
    for(int i=0; i<supprimer.size(); i++){
        this->dossier.remove(dossier.path()+"/"+supprimer.at(i));
    }
    supprimer.clear();

    //2) Copying
    for(int i=0; i<imprimer.size(); i++){
        QFile::copy(dossier.path()+"/"+imprimer.at(i), this->printedPictures.absolutePath()+"/"+imprimer.at(i));
    }
    imprimer.clear();

    //3) Moving
    for(int i=0; i<visitees.size(); i++){
        QFile photo; photo.setFileName(dossier.path()+"/"+visitees.at(i));
        if(photo.exists()){
            photo.rename(this->viewedPictures.absolutePath()+"/"+visitees.at(i));
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
    QMessageBox::about(this, "À propos de dadaPhoto", "<h3>À propos de dadaPhoto</h3><b>Version: </b>0.5.1<br><b>Créé par: </b>David Lumaye");
}

void dadaPhoto::chooseDir(){
    QString chemin = QFileDialog::getExistingDirectory(this);
    if(!chemin.isEmpty() && !chemin.isNull()){
        dossier.setPath(chemin);
    }
    else{
        return;
    }
    this->setPictureList();
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
        qApp->quit();
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
    // Read the JPEG file into a buffer
    FILE *fp = fopen(nomImage.c_str(), "rb");
    if (!fp) {
      printf("Can't open file.\n");
      return;
    }
    fseek(fp, 0, SEEK_END);
    unsigned long fsize = ftell(fp);
    rewind(fp);
    unsigned char *buf = new unsigned char[fsize];
    if (fread(buf, 1, fsize, fp) != fsize) {
      printf("Can't read file.\n");
      delete[] buf;
      return;
    }
    fclose(fp);

    // Parse EXIF
    easyexif::EXIFInfo result;
    int code = result.parseFrom(buf, fsize);
    delete[] buf;
    if (code) {
      printf("Error parsing EXIF: code %d\n", code);
      return;
    }

    if (result.Orientation == 6) {
        this->rotateRight();
    }

    return;
}

void dadaPhoto::importPictures() {
#ifdef Q_OS_LINUX
    return this->importPicturesLinux();
#else
    return this->importPicturesWindows();
#endif
}

void dadaPhoto::importPicturesWindows(){
    QMessageBox::warning(this, "Impossible d'importer", QString("Désolé… Impossible d'importer tes photos depuis Windows.  Copie-colle tes fichiers directement dans ce répertoire : %1").arg(this->dossier.absolutePath()));
    return;
}

void dadaPhoto::importPicturesLinux(){
    gphoto2 = new QProcess;
    gphoto2->start("gphoto2", QStringList("-v"));
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

    gphoto2->start("/home/leonard/download.sh", QStringList());
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

void dadaPhoto::detectDirtyDirectory(){
    //Détecte s'il y a des photos doublon et alerte l'utilisateur
    bool doublons = false;
    dossier.setNameFilters(QStringList()<<"*.jpg"<<"*.JPG"<<"*.png"<<"*.PNG"<<"*.jpeg"<<"*.JPEG"<<"*.raw"<<"*.RAW"<<"*.gif"<<"*.GIF"<<"*.bmp"<<"*.BMP");
    QStringList toShow = dossier.entryList(QDir::Files);
    QDir dirDossier =  QDir(QDir::homePath()+"/Images/Photos_vues");
    if(!dirDossier.exists()){
        return;
    }
    dirDossier.setNameFilters(QStringList()<<"*.jpg"<<"*.JPG"<<"*.png"<<"*.PNG"<<"*.jpeg"<<"*.JPEG"<<"*.raw"<<"*.RAW"<<"*.gif"<<"*.GIF"<<"*.bmp"<<"*.BMP");
    QStringList showed = dirDossier.entryList(QDir::Files);
    foreach(const QString &str, toShow){
        if(showed.contains(str)){
            doublons = true;
            break;
        }
    }

    if(doublons){
        int reponse = QMessageBox::information(this, "Photos en double", "Des photos <b>en double</b> ont été trouvées.<br />Cela signifie qu'elles sont <b>à la fois</b> vue <b>et</b> non-vues.  D'où, un petit casse-tête.<br />Si tu le souhaites, <i>dadaPhotos</i> peut supprimer les doublons.<br />Concrètement, qu'est-ce qui va se passer?<br /><i>dadaPhotos</i> va supprimer des photos à voir toutes celles qui ont déjà été vues.  Tu ne perdras <b>aucune</b> photo.<br /><b>NOTE:</b>Suite à cette opération, il se peut que tu revoies des photos vues que tu as supprimées dans le passé.  Dans ce cas, il faut les supprimer à nouveau.<br />Souhaites-tu nettoyer les doublons?", QMessageBox::Yes | QMessageBox::No);
        if(reponse == QMessageBox::Yes){
            this->cleanDirectory();
        }
    }
}

void dadaPhoto::cleanDirectory(){
    int nombre = 0;
    //Cette fonction supprimer les doublons du répertoire de base et du répertoire de photos vues.  Les photos vues sont prioritaires
    dossier.setNameFilters(QStringList()<<"*.jpg"<<"*.JPG"<<"*.png"<<"*.PNG"<<"*.jpeg"<<"*.JPEG"<<"*.raw"<<"*.RAW"<<"*.gif"<<"*.GIF"<<"*.bmp"<<"*.BMP");
    QStringList toShow = dossier.entryList(QDir::Files);
    QDir dirDossier =  QDir(QDir::homePath()+"/Images/Photos_vues");
    if(!dirDossier.exists()){
        return;
    }
    dirDossier.setNameFilters(QStringList()<<"*.jpg"<<"*.JPG"<<"*.png"<<"*.PNG"<<"*.jpeg"<<"*.JPEG"<<"*.raw"<<"*.RAW"<<"*.gif"<<"*.GIF"<<"*.bmp"<<"*.BMP");
    QStringList showed = dirDossier.entryList(QDir::Files);
    foreach(const QString &str, toShow){
        if(showed.contains(str)){
            QFile toDelete(dossier.path()+"/"+str);
            toDelete.remove();
            nombre++;
        }
    }

    //Et on réactualise la liste d'image
    ui->listWidget->clear();
    this->setPictureList();

    //On affiche un message
    QMessageBox::information(this, "Nettoyage terminé", "Voilà, "+QString::number(nombre)+" photos ont été supprimées.");
    return;
}

void dadaPhoto::close(){
    if(visitees.size() > 1){
        int reponse = QMessageBox::warning(this, "Changement non enregistrés", "<b>ATTENTION</b>   Tu as regardé des images sans appuyer sur «valider».  Souhaites-tu valider les changements effectués?", QMessageBox::Yes | QMessageBox::No);
        if(reponse == QMessageBox::Yes){
            this->apply();
        }
    }
    return;
}

/**
 * @brief dadaPhoto::prepareDestinationDirectories
 * @internal
 */
void dadaPhoto::prepareDestinationDirectories() {
    this->viewedPictures = this->getWritableLocation(this->dossier, dadaPhoto::VIEWED_PICTURES_DIRECTORY_NAME);
    this->printedPictures = this->getWritableLocation(this->dossier, dadaPhoto::PRINTING_PICTURES_DIRECTORY_NAME);
}

/**
 * @brief dadaPhoto::getWritableLocation
 * @internal
 * @param rootLocation
 * @param wantedLocation
 * @return
 */
QDir dadaPhoto::getWritableLocation(QDir rootLocation, QString wantedLocation) {
    QDir targetDirectory = QDir(rootLocation.absolutePath()+"/"+wantedLocation);

    // If directory already exists
    if (targetDirectory.exists()) {
        QFileInfo targetDirectoryInfo = QFileInfo(targetDirectory.absolutePath());
        // It is writable, perfect
        if (targetDirectoryInfo.isWritable()) {
            return targetDirectory;
        }
        // Not writable, requesting alternate location
        return this->getFallbackDirectory(wantedLocation);
    }

    // If here, directory doesn't exists
    bool creationSuccessFull = targetDirectory.mkdir(targetDirectory.absolutePath());

    if (creationSuccessFull) {
        return targetDirectory;
    }

    // If here, unable to create directory
    return this->getFallbackDirectory(wantedLocation);
}

/**
 * @brief dadaPhoto::getFallbackDirectory
 * @internal
 * @param wantedLocation
 * @return
 */
QDir dadaPhoto::getFallbackDirectory(QString wantedLocation) {
    QDir tempDirectory = QDir(QStandardPaths::writableLocation(QStandardPaths::TempLocation));
    QDir fallbackDirectory = QDir(tempDirectory.absolutePath()+"/"+wantedLocation);
    // Check for writability in temp location
    QFileInfo fallbackDirectoryInfo = QFileInfo(fallbackDirectory.absolutePath());
    if (fallbackDirectoryInfo.isWritable()) {
        bool newDirectory = fallbackDirectory.mkdir(fallbackDirectory.absolutePath()+"/"+wantedLocation);
        if (newDirectory) {
            // A warning because fallback is working
            QMessageBox::warning(this, tr("Dossier non inscriptible"), tr("Le dossier «%1» n'est pas accessible en écriture.  Les données seront mises dans %2").arg(fallbackDirectory.absolutePath()).arg(fallbackDirectory.absolutePath()));
            return fallbackDirectory;
        }
    }
    // Fatal failure
    QMessageBox::critical(this, tr("Dossier non inscriptible"), tr("Aucun dossier n'est accessible en écriture.  Les changements ne seront donc *PAS* pris en compte."));
    return QDir();
}
