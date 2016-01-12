#include <QtCore>
#include "SpriteAtlas.h"
#include "PublishSpriteSheet.h"
#include "SpritePackerProjectFile.h"

int commandLine(QCoreApplication& app) {
    QCommandLineParser parser;
    parser.setApplicationDescription("");
    parser.addHelpOption();
    parser.addVersionOption();
    parser.addPositionalArgument("source", "Sprites for packing or project file (You can override project file options with [options]).");
    parser.addPositionalArgument("destination", "Destination folder where you're saving the sprite sheet. Optional when using project file");

    parser.addOptions({
        {{"f", "format"}, "Format for export sprite sheet data. Default is cocos2d.", "format"},
        {"trimMode", "Rect - Removes the transparency around a sprite. The sprites appear to have their original size when using them.\n\
Polygon - The amount of rendered transparency can be reduced by creating a tight fitting polygon around the solid pixels of a sprite. But: The vertices must be transformed by the CPU — introducing new costs.\n\
Default is Rect", "mode", "Rect"},
        {"trim", "Allowed values: 1 to 255, default is 1. Pixels with an alpha value below this value will be considered transparent when trimming the sprite. Very useful for sprites with nearly invisible alpha pixels at the borders.", "int", "1"},
        {"epsilon", "Lower values create a tighter fitting mesh with less transparency but with more vertices.\nHigher values on the other hand reduce the number of vertices at the cost of adding more transparency.", "float", "5"},
        {"texture-border", "Border of the sprite sheet, value adds transparent pixels around the borders of the sprite sheet. Default value is 0.", "int", "0"},
        {"sprite-border", "Sprite border is the space between sprites. Value adds transparent pixels between sprites to avoid artifacts from neighbor sprites. The transparent pixels are not added to the sprites, default is 2.", "int", "2"},
        {"powerOf2", "Forces the texture to have power of 2 size (32, 64, 128...). Default is disable."},
        {"max-size", "Sets the maximum size for the texture, default is 8192.", "size", "8192"},
        {"opt-level", "Optimizes the image's file size. Allowed values: 1 to 7 (Higher values take noticeably longer.", "int", "0"},
        {"scale", "Scales all images before creating the sheet. E.g. use 0.5 for half size, default is 1 (Scale has no effect when source is a project file).", "float", "1"},
    });

    //--texture-border 10 /Users/alekseymakaseev/Documents/Work/run-and-jump/Assets/ART/Character /Users/alekseymakaseev/Documents/Work/run-and-jump/RunAndJump/testResources --trim 2
    parser.process(app);

    bool destinationSet = true;
    SpritePackerProjectFile* projectFile = nullptr;

    if (parser.positionalArguments().size() > 2) {
        qDebug() << "Too many arguments, see help for information.";
        parser.showHelp();
        return -1;
    } else if ((parser.positionalArguments().size() == 1) || (parser.positionalArguments().size() == 2)) {
        QFileInfo src(parser.positionalArguments().at(0));
        if (!src.isDir()) {
            std::string suffix = src.suffix().toStdString();
            projectFile = SpritePackerProjectFile::factory().get(suffix)();
        }

        if (parser.positionalArguments().size() == 1) {
            if (!projectFile) {
                qDebug() << "Arguments must have source and destination, see help for information.";
                parser.showHelp();
                return -1;
            } else {
                destinationSet = false; // we should already have our destination saved in our project file
            }
        } else if (parser.positionalArguments().size() == 2) {
            destinationSet = true;
        }

    } else {
        qDebug() << "Arguments must have source and destination, see help for information.";
        parser.showHelp();
        return -1;
    }

    qDebug() << "arguments:" << parser.positionalArguments();
    qDebug() << "options:" << parser.optionNames();

    QFileInfo source(parser.positionalArguments().at(0));
    QFileInfo destination;

    if (destinationSet) {
        destination.setFile(parser.positionalArguments().at(1));
    }

    // initialize [options]
    QString trimMode = "Rect";
    int trim = 1;
    float epsilon = 5.f;
    int textureBorder = 0;
    int spriteBorder = 2;
    bool pow2 = false;
    int maxSize = 8192;
    float imageScale = 1;
    QString format = "cocos2d";
    int optLevel = 0;

    if (projectFile) {
        if (!projectFile->read(source.filePath())) {
            qCritical() << "File format error.";
        } else {
            trimMode = projectFile->trimMode();
            trim = projectFile->trimThreshold();
            epsilon = projectFile->epsilon();
            textureBorder = projectFile->textureBorder();
            spriteBorder = projectFile->spriteBorder();
            pow2 = projectFile->pow2();
            maxSize = projectFile->maxTextureSize();
            optLevel = projectFile->optLevel();

            if (!destinationSet) {
                destination.setFile(projectFile->destPath());
            }
        }
    }

    if (!destination.filePath().endsWith("/")) {
        destination.setFile(destination.filePath() + "/");
    }

    if (!destination.isDir()) {
        qDebug() << "Incorrect destination folder";
        return -1;
    }

    // you can override project file options
    if (parser.isSet("trimMode")) {
        trimMode = parser.value("trimMode");
    }
    if (parser.isSet("trim")) {
        trim = parser.value("trim").toInt();
    }
    if (parser.isSet("epsilon")) {
        epsilon = parser.value("epsilon").toFloat();
    }
    if (parser.isSet("texture-border")) {
        textureBorder = parser.value("texture-border").toInt();
    }
    if (parser.isSet("sprite-border")) {
        spriteBorder = parser.value("sprite-border").toInt();
    }
    if (parser.isSet("powerOf2")) {
        pow2 = true;
    }
    if (parser.isSet("max-size")) {
        maxSize = parser.value("max-size").toInt();
    }
    if (parser.isSet("scale") && !projectFile) {
        imageScale = parser.value("scale").toFloat();
    }
    if (parser.isSet("format")) {
        format = parser.value("format");
    }

    if (parser.isSet("opt-level")) {
        optLevel = parser.value("opt-level").toInt();
    }

    qDebug() << "trimMode:" << trimMode;
    qDebug() << "trim:" << trim;
    qDebug() << "epsilon:" << epsilon;
    qDebug() << "textureBorder:" << textureBorder;
    qDebug() << "spriteBorder:" << spriteBorder;
    qDebug() << "pow2:" << pow2;
    qDebug() << "maxSize:" << maxSize;
    qDebug() << "scale:" << imageScale;
    qDebug() << "opt-level:" << optLevel;

    // load formats
    QSettings settings;
    QStringList formatsFolder;
    formatsFolder.push_back(QCoreApplication::applicationDirPath() + "/defaultFormats");
    formatsFolder.push_back(settings.value("Preferences/customFormatFolder").toString());

    // load formats
    PublishSpriteSheet::formats().clear();
    for (auto folder: formatsFolder) {
        if (QDir(folder).exists()) {
            QDirIterator fileNames(folder, QStringList() << "*.js", QDir::Files | QDir::NoSymLinks | QDir::NoDotAndDotDot);
            while(fileNames.hasNext()) {
                fileNames.next();
                PublishSpriteSheet::addFormat(fileNames.fileInfo().baseName(), fileNames.filePath());
            }
        }
    }
    qDebug() << "Support Formats:" << PublishSpriteSheet::formats().keys();

    if (projectFile) {
        for (int i=0; i<projectFile->scalingVariants().size(); ++i) {
            ScalingVariant variant = projectFile->scalingVariants().at(i);

            QString variantName = variant.name;
            float scale = variant.scale;

            QString spriteSheetName = projectFile->spriteSheetName();
            if (spriteSheetName.contains("{v}")) {
                spriteSheetName.replace("{v}", variantName);
            } else {
                spriteSheetName = variantName + spriteSheetName;
            }
            while (spriteSheetName.at(0) == '/') {
                spriteSheetName.remove(0,1);
            }

            QFileInfo destFileInfo;
            destFileInfo.setFile(destination.dir(), spriteSheetName);
            if (destination.absolutePath() != destFileInfo.dir().absolutePath()) {
                if (!destination.dir().mkpath(destFileInfo.dir().absolutePath())) {
                    qWarning() << "Imposible create path:" + destFileInfo.dir().absolutePath();
                    continue;
                }
            }

            // Generate sprite atlas
            SpriteAtlas atlas(QStringList() << projectFile->srcList(), textureBorder, spriteBorder, trim, pow2, maxSize, scale);
            if (trimMode == "Polygon") {
                atlas.enablePolygonMode(true, epsilon);
            }
            if (!atlas.generate()) {
                qCritical() << "ERROR: Generate atlas!";
                return -1;
            }

            // Publish data
            if (!PublishSpriteSheet::publish(destFileInfo.filePath(), format, optLevel, atlas, false)) {
                qCritical() << "ERROR: publish atlas!";
                return -1;
            }
        }

        delete projectFile;
        projectFile = nullptr;
    } else {
        // Generate sprite atlas
        SpriteAtlas atlas(QStringList() << source.filePath(), textureBorder, spriteBorder, trim, pow2, maxSize, imageScale);
        if (trimMode == "Polygon") {
            atlas.enablePolygonMode(true, epsilon);
        }
        if (!atlas.generate()) {
            qCritical() << "ERROR: Generate atlas!";
            return -1;
        }

        // Publish data
        if (!PublishSpriteSheet::publish(destination.filePath() + source.fileName(), format, optLevel, atlas, false)) {
            qCritical() << "ERROR: publish atlas!";
            return -1;
        }
    }
    qDebug() << "Publishing is finished.";


//    qDebug() << source.fileName() << source.isDir();
//    qDebug() << destination.filePath() << destination.isDir();

    return 1;
}

