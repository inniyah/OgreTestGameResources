/*
-----------------------------------------------------------------------------
This source file is part of
                                    _
  ___   __ _ _ __ ___  __ _ ___ ___(_)_ __ ___  _ __
 / _ \ / _` | '__/ _ \/ _` / __/ __| | '_ ` _ \| '_ \
| (_) | (_| | | |  __/ (_| \__ \__ \ | | | | | | |_) |
 \___/ \__, |_|  \___|\__,_|___/___/_|_| |_| |_| .__/
       |___/                                   |_|

For the latest info, see https://bitbucket.org/jacmoe/ogreassimp

Copyright (c) 2011 Jacob 'jacmoe' Moen

Licensed under the MIT license:

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in
all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
THE SOFTWARE.
-----------------------------------------------------------------------------
*/
#include <iostream>
#include <sys/stat.h>
#include <random>

#include <Ogre.h>
#include <OgreString.h>
#include <OgreMeshSerializer.h>
#include <OgreSkeletonSerializer.h>
#include <OgreLodStrategyManager.h>
#include <OgreDefaultHardwareBufferManager.h>
#include <OgreScriptCompiler.h>
#include <OgreFileSystem.h>

#ifndef DONT_USE_XML
#include <OgreXMLMeshSerializer.h>
#include <OgreXMLSkeletonSerializer.h>
#endif // DONT_USE_XML

#include "AssimpLoader.h"

namespace uuid {
    static std::random_device              rd;
    static std::mt19937                    gen(rd());
    static std::uniform_int_distribution<> dis(0, 15);
    static std::uniform_int_distribution<> dis2(8, 11);

    std::string generate_uuid_v4() {
        std::stringstream ss;
        int i;
        ss << std::hex;
        for (i = 0; i < 8; i++) {
            ss << dis(gen);
        }
        ss << "-";
        for (i = 0; i < 4; i++) {
            ss << dis(gen);
        }
        ss << "-4";
        for (i = 0; i < 3; i++) {
            ss << dis(gen);
        }
        ss << "-";
        ss << dis2(gen);
        for (i = 0; i < 3; i++) {
            ss << dis(gen);
        }
        ss << "-";
        for (i = 0; i < 12; i++) {
            ss << dis(gen);
        };
        return ss.str();
    }
}

void help(void)
{
    // Print help message
    std::cout << std::endl << "OgreAssimpConverter: Converts data from model formats supported by Assimp" << std::endl;
    std::cout << "to OGRE binary formats (mesh and skeleton) and material script." << std::endl;
    std::cout << std::endl << "Usage: OgreAssimpConverter [options] sourcefile [destination] " << std::endl;
    std::cout << std::endl << "Available options:" << std::endl;
    std::cout << "-q                  = Quiet mode, less output" << std::endl;
    std::cout << "-log filename       = name of the log file (default: 'ass.log')" << std::endl;
    std::cout << "-aniSpeedMod value  = Factor to scale the animation speed - (default: '1.0')" << std::endl;
    std::cout << "                      (double between 0 and 1)" << std::endl;
    std::cout << "-3ds_ani_fix        = Fix for the fact that 3ds max exports the animation over a" << std::endl;
    std::cout << "                      longer time frame than the animation actually plays for" << std::endl;
    std::cout << "-3ds_dae_fix        = When 3ds max exports as DAE it gets some of the transforms wrong, get around this" << std::endl;
    std::cout << "                      by using this option and a prior run with of the model exported as ASE" << std::endl;
    std::cout << "-shader             = create shader based materials" << std::endl;
    std::cout << "-shadows            = set receive shadows = on in material script" << std::endl;

#ifndef DONT_USE_LOD
    std::cout << "-l lodlevels        = number of LOD levels" << std::endl;
    std::cout << "-v lodvalue         = value increment to reduce LOD" << std::endl;
    std::cout << "-s lodstrategy      = LOD strategy to use for this mesh" << std::endl;
    std::cout << "-p lodpercent       = Percentage triangle reduction amount per LOD" << std::endl;
    std::cout << "-f lodnumtris       = Fixed vertex reduction per LOD" << std::endl;
#endif // DONT_USE_LOD

    std::cout << "-prefix             = prefix to add to the output filename" << std::endl;
    std::cout << "-suffix             = suffix to add to the output filename" << std::endl;
    std::cout << "-uuid               = uuid to use to make names unique" << std::endl;

    std::cout << "sourcefile          = name of file to convert" << std::endl;
    std::cout << "destination         = optional name of directory to write to. If you don't" << std::endl;
    std::cout << "                      specify this the converter will use the same directory as the sourcefile."  << std::endl;
    std::cout << std::endl;
}

AssimpLoader::AssOptions parseArgs(int numArgs, char **args)
{
    AssimpLoader::AssOptions opts;
    opts.quietMode = false;
    opts.logFile = "ass.log";
    opts.customAnimationName = "";
    opts.dest = "";
    opts.namePrefix = "";
    opts.nameSuffix = ""; //  "_" + extension;
    opts.uuid = "";
    opts.animationSpeedModifier = 1.0;

#ifndef DONT_USE_LOD
    opts.lodValue = 250000;
    opts.lodFixed = 0;
    opts.lodPercent = 20;
    opts.numLods = 0;
    opts.usePercent = true;
#endif // DONT_USE_LOD

    // ignore program name
    char* source = 0;
    char* dest = 0;

    // Set up options
    Ogre::UnaryOptionList unOpt;
    Ogre::BinaryOptionList binOpt;

    unOpt["-q"] = false;
    unOpt["-3ds_ani_fix"] = false;
    unOpt["-3ds_dae_fix"] = false;
    unOpt["-shader"] = false;
    binOpt["-log"] = "ass.log";
    binOpt["-aniName"] = "";
    binOpt["-aniSpeedMod"] = "1.0";
    binOpt["-prefix"] = "";
    binOpt["-suffix"] = "";
    binOpt["-uuid"] = uuid::generate_uuid_v4();

#ifndef DONT_USE_LOD
    binOpt["-l"] = "";
    binOpt["-v"] = "";
    binOpt["-s"] = "Distance";
    binOpt["-p"] = "";
    binOpt["-f"] = "";
#endif // DONT_USE_LOD

    int startIndex = Ogre::findCommandLineOpts(numArgs, args, unOpt, binOpt);
    Ogre::UnaryOptionList::iterator ui;
    Ogre::BinaryOptionList::iterator bi;

    opts.params = AssimpLoader::LP_GENERATE_SINGLE_MESH;

    ui = unOpt.find("-q");
    if (ui->second)
    {
        opts.quietMode = true;
    }
    ui = unOpt.find("-3ds_ani_fix");
    if (ui->second)
    {
        opts.params |= AssimpLoader::LP_CUT_ANIMATION_WHERE_NO_FURTHER_CHANGE;
    }
    ui = unOpt.find("-3ds_dae_fix");
    if (ui->second)
    {
        opts.params |= AssimpLoader::LP_USE_LAST_RUN_NODE_DERIVED_TRANSFORMS;
    }
    ui = unOpt.find("-shader");
    if (ui->second)
    {
        opts.params |= AssimpLoader::LP_GENERATE_SHADER_MATERIALS;
    }

    bi = binOpt.find("-log");
    if (!bi->second.empty())
    {
        opts.logFile = bi->second;
    }
    bi = binOpt.find("-aniSpeedMod");
    if (!bi->second.empty())
    {
        opts.animationSpeedModifier = Ogre::StringConverter::parseReal(bi->second);
    }
    bi = binOpt.find("-aniName");
    if (!bi->second.empty())
    {
        opts.customAnimationName = bi->second;
    }

#ifndef DONT_USE_LOD
    bi = binOpt.find("-l");
    if (!bi->second.empty())
    {
        opts.numLods = Ogre::StringConverter::parseInt(bi->second);
    }
    bi = binOpt.find("-v");
    if (!bi->second.empty())
    {
        opts.lodValue = Ogre::StringConverter::parseReal(bi->second);
    }
    bi = binOpt.find("-s");
    if (!bi->second.empty())
    {
        opts.lodStrategy = bi->second;
    }
    bi = binOpt.find("-p");
    if (!bi->second.empty())
    {
        opts.lodPercent = Ogre::StringConverter::parseReal(bi->second);
        opts.usePercent = true;
    }
    bi = binOpt.find("-f");
    if (!bi->second.empty())
    {
        opts.lodFixed = Ogre::StringConverter::parseInt(bi->second);
        opts.usePercent = false;
    }
#endif // DONT_USE_LOD

    bi = binOpt.find("-prefix");
    if (!bi->second.empty())
    {
        opts.namePrefix = bi->second;
    }
    bi = binOpt.find("-suffix");
    if (!bi->second.empty())
    {
        opts.nameSuffix = bi->second;
    }
    bi = binOpt.find("-uuid");
    if (!bi->second.empty())
    {
        opts.uuid = bi->second;
    }

    // Source / dest
    if (numArgs > startIndex)
        source = args[startIndex];
    if (numArgs > startIndex+1)
        dest = args[startIndex+1];
    if (numArgs > startIndex+2) {
        std::cout << "Too many command-line arguments supplied - abort. " << std::endl;
        help();
        exit(1);
    }

    if (!source)
    {
        std::cout << "Missing source file - abort. " << std::endl;
        help();
        exit(1);
    }
    opts.source = source;

    if (dest)
    {
        opts.dest = dest;
    }

    if (!opts.quietMode)
    {
        std::cout << std::endl;
        std::cout << "-- OPTIONS --" << std::endl;

        std::cout << "source file               = " << opts.source << std::endl;
        std::cout << "destination               = " << opts.dest << std::endl;
        std::cout << "animation speed modifier  = " << opts.animationSpeedModifier << std::endl;
        std::cout << "log file                  = " << opts.logFile << std::endl;

        std::cout << "-- END OPTIONS --" << std::endl;
        std::cout << std::endl;
    }

    return opts;
}

// Crappy globals
// NB some of these are not directly used, but are required to
//   instantiate the singletons used in the dlls
Ogre::LogManager* logMgr = 0;
Ogre::Math* mth = 0;
Ogre::LodStrategyManager *lodMgr = 0;
Ogre::MaterialManager* matMgr = 0;
Ogre::SkeletonManager* skelMgr = 0;
Ogre::MeshSerializer* meshSerializer = 0;
Ogre::SkeletonSerializer* skeletonSerializer = 0;

#ifndef DONT_USE_XML
Ogre::XMLMeshSerializer* xmlMeshSerializer = 0;
Ogre::XMLSkeletonSerializer* xmlSkeletonSerializer = 0;
#endif // DONT_USE_XML

Ogre::DefaultHardwareBufferManager *bufferManager = 0;
Ogre::MeshManager* meshMgr = 0;
Ogre::ResourceGroupManager* rgm = 0;
Ogre::ScriptCompilerManager* scmgr = 0;
Ogre::ArchiveManager* archmgr = 0;
Ogre::FileSystemArchiveFactory* mfsarchf = 0;

Ogre::DefaultTextureManager* texMgr = 0;

int main(int numargs, char** args)
{
    if (numargs < 2)
    {
        help();
        return -1;
    }

    // Assume success
    int retCode = 0;

    try
    {
        logMgr = new Ogre::LogManager();

        // this log catches output from the parseArgs call and routes it to stdout only
        logMgr->createLog("Temporary log", false, true, true);

        AssimpLoader::AssOptions opts = parseArgs(numargs, args);
        // use the log specified by the cmdline params
        logMgr->setDefaultLog(logMgr->createLog(opts.logFile, false, true));
        // get rid of the temporary log as we use the new log now
        logMgr->destroyLog("Temporary log");

        rgm = new Ogre::ResourceGroupManager();
        mth = new Ogre::Math();
        lodMgr = new Ogre::LodStrategyManager();
        meshMgr = new Ogre::MeshManager();
        matMgr = new Ogre::MaterialManager();
        matMgr->initialise();
        skelMgr = new Ogre::SkeletonManager();
        meshSerializer = new Ogre::MeshSerializer();
        skeletonSerializer = new Ogre::SkeletonSerializer();

#ifndef DONT_USE_XML
        xmlMeshSerializer = new Ogre::XMLMeshSerializer();
        xmlSkeletonSerializer = new Ogre::XMLSkeletonSerializer();
#endif // DONT_USE_XML

        bufferManager = new Ogre::DefaultHardwareBufferManager(); // needed because we don't have a rendersystem
        scmgr = new Ogre::ScriptCompilerManager();
        archmgr = new Ogre::ArchiveManager();
        mfsarchf = new Ogre::FileSystemArchiveFactory();
        Ogre::ArchiveManager::getSingleton().addArchiveFactory( mfsarchf );

        texMgr = new Ogre::DefaultTextureManager();

        if(opts.quietMode)
            opts.params |= AssimpLoader::LP_QUIET_MODE;

        AssimpLoader loader;
        loader.convert(opts);

    }
    catch(Ogre::Exception& e)
    {
        std::cerr << "FATAL ERROR: " << e.getDescription() << std::endl;
        std::cerr << "ABORTING!" << std::endl;
        retCode = 1;
    }

#ifndef DONT_USE_XML
    delete xmlSkeletonSerializer;
    delete xmlMeshSerializer;
#endif // DONT_USE_XML

    delete skeletonSerializer;
    delete meshSerializer;
    delete skelMgr;
    delete matMgr;
    //delete meshMgr; FIX this!!
    delete bufferManager;
    delete scmgr;
    delete archmgr;
    delete mfsarchf;
    delete lodMgr;
    delete mth;
    delete rgm;
    delete logMgr;

    return retCode;

}

