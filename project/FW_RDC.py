import sys
import os
import datetime
import subprocess
import shutil
from distutils.dir_util import copy_tree
import time
import distutils
import os.path
import fnmatch

# Global vars #################################################################################

strCurDir = os.getcwd()
strOutputDir = ""
strAppName=""
strAppVersion=""
strPlatform=""
strPackType=""
strCompile=""
strProdDev=""
strToolChainPath=""
strSDKPath=""
strEOSPath=""
strADKPath=""
strCompilerPath=""
strLinkerPath=""
strCompilerOptions=""
strLinkerOptions=""
strSDKLibPath=""
strSDKIncludes=""
strEOSIncludes=""
strCTLSLibPath=""
strResourcesFolder= strCurDir + "\\..\\resources"
strTarFileName=""
strADKlibPath=""
lstBundleFiles = []

strNetDown="C:\\Progra~2\\VeriFone\\Mx800Downloader"


# Common functions / structures ###############################################################

class FileInfo():
	def __init__(self, name, signed, extractTGZ, GID, drive):
		self.name = name
		self.signed = signed
		self.extractTGZ = extractTGZ
		self.GID = GID
		self.drive = drive

def createFoldersStructure():
	global strOutputDir
	if(strPackType=="NOPACK" ):
		return
	
	os.chdir(strCurDir)

	if os.path.exists(".\\obj"):
		wraprmtree(".\\obj")
		time.sleep(1)
	os.makedirs(".\\obj")

	if(strCompile != "NO"):
		if os.path.exists(".\\out"):
			wraprmtree(".\\out")
			time.sleep(1)	
		os.makedirs(".\\out")
	
	if os.path.exists(".\\bundle"):
		wraprmtree(".\\bundle")
		time.sleep(1)
	
	os.makedirs(".\\bundle")
	os.chdir(".\\bundle")
	os.makedirs(".\\" + strPlatform)
	os.chdir(".\\" + strPlatform)
	os.makedirs(".\\temp1")
	os.makedirs(".\\temp2")
	os.makedirs(".\\temp3")
	strOutputDir = os.getcwd()
	os.chdir(strOutputDir)
	os.makedirs(".\\crt")
	#os.makedirs(".\\CONTROL")
	os.chdir(strCurDir)
	
def wraprmtree(pahtName):
	while True:
		try:
			shutil.rmtree(pahtName)
		except OSError: 
			#os.system("cls")
			#os.system("pause")
			print (" PROCESSING ...\n PLEASE WAIT ")
		else:
			break;

#----------------------------------------------------------------------------------------------

def setEVOConfig():
	global srtCygwinTool
	global strToolChainPath
	global strSDKPath
	global strEOSPath
	global strADKPath
	global strCompilerPath
	global strLinkerPath
	global strCompilerOptions
	global strLinkerOptions
	global strSDKLibPath
	global strSDKIncludes
	global strEOSIncludes
	global strCTLSLibPath
	
	srtCygwinTool = r"C:\Progra~2\VERIFONE\PackageManagerProduction\Cygwin"
	strToolChainPath = r"C:\vde\SDKs\vrx\VRX_3_9_5"
	strSDKPath = r"C:\vde\SDKs\vrx\VRX_3_9_5"
	
	strSDKLibPath = strSDKPath + r"\lib"
	strSDKIncludes = strSDKPath + r"\include"
	strEOSPath = r"C:\eVoAps\EOS\EOSSDK011-02080301"
	strEOSIncludes = strEOSPath + r"\include"
	strADKPath = r"C:\vde\ADKs\adk-full-4.2.2.1-213\vrx"
	strCTLSLibPath = r"C:\eVoAps\CTLS_SDK\01.00.01.10\lib"
	strCompilerPath = strToolChainPath + r"\bin\vrxcc.exe"
	strLinkerPath = strToolChainPath + r"\bin\vrxcc.exe" 
	strCompilerOptions = r"-c -p -g -O2 -vsoapp -armcc,--exceptions -armcc,--diag-suppress=1  -armcc,--diag-suppress=1300 -armcc,--diag-suppress=1299 -armcc,--diag-suppress=177 -DLOGAPI_ENABLE_DEBUG -D_DEBUG -DVFI_GUIPRT_IMPORT -DVFI_IPC_DLL_IMPORT -DVFI_SYSINFO_DLL_IMPORT -DVFI_SYSBAR_DLL_IMPORT -DVFI_MAC_DLL_IMPORT -D_VRXEVO -DVFI_COM_DLL_IMPORT -D_EXCEPTION_LOGGING -DLIB_CONFIG -DVFI_GUIPRT_IMPORT -DVFI_SYSBAR_DLL_IMPORT -DVFI_POSIX_IMPORT -DVFI_SEC_DLL_IMPORT"
	strLinkerOptions = r"-vsoapp -g -p -map -symbols -xref"
#----------------------------------------------------------------------------------------------

def setVOSConfig():
	global strToolChainPath
	global srtCygwinTool
	global strSDKPath
	global strADKPath
	global strCompilerPath
	global strLinkerPath
	global strCompilerOptions
	global strLinkerOptions
	global strSDKLibPath
	global strADKlibPath
	
	
	srtCygwinTool = r"C:\Progra~2\VERIFONE\PackageManagerProduction\Cygwin"
	#TOOLCHAIN / ADK / SDK PATH
	strToolChainPath = r"C:\vde\toolchains\windows\vos"
	#strSDKPath = r"C:\vde\SDKs\vos\Mx9xxSDK-1.2.7-DTK400.410904"
	#strADKPath = r"\vde\ADKs\adk-full-4.3.5-416\vos"
	strSDKPath = r"C:\vde\SDKs\vos\vos-sdk-winx86-release-30410900\Mx9xxSDK-1.2.7-DTK400.410802"
	strADKPath = r"C:\vde\ADKs\adk-full-4.3.4-369\vos"
	
	strSDKLibPath = strSDKPath + r"\usr\local\lib"
	strADKlibPath = strADKPath + r"\\lib"
	strCompilerPath = strToolChainPath + r"\arm-verifone-linux-gnueabi\bin\arm-verifone-linux-gnueabi-g++.exe"
	strLinkerPath = strToolChainPath + r"\arm-verifone-linux-gnueabi\bin\arm-verifone-linux-gnueabi-g++.exe"
	strCompilerOptions = r"-c -Wno-write-strings -D__VOS_DEBUG -D__arm -pthread -DTIXML_USE_STL"
	strLinkerOptions = r"--sysroot=" + strSDKPath + " -Wl,-rpath=" + strSDKPath + r"\usr\local\lib\svcmgr -W1, -unresolved-symbols=ignore-in-shared-libs"

#----------------------------------------------------------------------------------------------

def setVOS2Config():
	global strToolChainPath
	global srtCygwinTool
	global strSDKPath
	global strADKPath
	global strCompilerPath
	global strLinkerPath
	global strCompilerOptions
	global strLinkerOptions
	global strSDKLibPath
	global strADKlibPath
	

	#vos2_sdk_ver="30811100"
	#vos2_sdk_ver="30811900"
	#vos2_sdk_ver="30812100"
	#vos2_sdk_ver="31342700"
	#vos2_adk_ver="4.4.7-259"
	#vos2_adk_ver="4.4.13-440"
	#vos2_adk_ver="4.4.15-498"
	#vos2_adk_ver="4.7.19-1342"

	#DEV JRS
	#vos2_sdk_ver="31343200-A200"
	#vos2_adk_ver="4.7.22.1-1443"

	vos2_sdk_ver="31343300-A200"
	vos2_adk_ver="4.7.23.1-1483"


	print("Setting up Verix VOS2 configuration...")
	
	#srtCygwinTool = r"C:\Progra~2\VeriFone\PackageManagerProduction\Cygwin"
	srtCygwinTool = r"C:\Cygwin"
	#TOOLCHAIN / ADK / SDK PATH
	strToolChainPath = "C:/" + "/vde/toolchains/windows/vos2"
	strSDKPath = "C:/" +"vde/SDKs/vos2/vos2-sdk-winx86-release-" + vos2_sdk_ver + "/vos2"
	strADKPath = "C:/" + "vde/ADKs/adk-full-" + vos2_adk_ver + "/vos2"
	strSDKLibPath = strSDKPath + r"\usr\local\lib"
	strADKlibPath = strADKPath + r"\\lib"
	strCompilerPath = strToolChainPath + "/gcc-linaro-arm-linux-gnueabihf-4.7-2013.03/bin/arm-linux-gnueabihf-g++.exe"
	strLinkerPath = strToolChainPath + "/gcc-linaro-arm-linux-gnueabihf-4.7-2013.03/bin/arm-linux-gnueabihf-g++.exe"   
	strCompilerOptions = r"-c -Wno-write-strings -D__VOS_LOGF_TRACE -D__arm -pthread -DTIXML_USE_STL"
	strLinkerOptions = r"-Wl,-rpath=" + strSDKPath + "/usr/lib -Wl,-rpath=" + strSDKPath + "/usr/local/lib -Wl,-rpath=" + strSDKPath + "/lib -Wl,-rpath=" + strSDKPath + "/usr/local/lib/svcmgr"


#----------------------------------------------------------------------------------------------

def setVOS2BaseFilesList():
	global strADKPath
	global strOutputDir

	if(strPackType=="NOPACK" ):
		return
	
	strSourceFolder = strADKPath + "\\load\\solutions\\"

	if(strPackType=="PACK1"):	
		lstBundleFiles.append(FileInfo("C:\\vde\\SDKs\\vos2\\dl.sdk\\dl.vos2-prod-V200c-Ctls-release-30411200", 0, 1, "", ""))		# OS. Must go first

	if(strPackType=="PACK1" or strPackType=="PACK2"):
		for file in os.listdir(strSourceFolder):
			if fnmatch.fnmatch(file, '*vos2-sys_all-prod.tgz'):
				print (file)
				lstBundleFiles.append(FileInfo(strSourceFolder + file, 0, 1, "", ""))
		
		strSourceFolder = strADKPath + "\\load\\solutions\\"        # signed by customer & scapp modified
		for file in os.listdir(strSourceFolder):
			if fnmatch.fnmatch(file, '*vos2-usr_all-prod.tgz'):
				print (file)
				lstBundleFiles.append(FileInfo(strSourceFolder + file,1,1,"",""))	

		### mac remove in final touches
		#strSourceFolder = strADKPath + "\\load\\sysinfo\\"				
		#for file in os.listdir(strSourceFolder):
			#	if fnmatch.fnmatch(file, 'dl.mac-remove*prod.tgz'):
		#		print file	
		#		lstBundleFiles.append(FileInfo(strSourceFolder + file, 0, 1, "", ""))

		### scapp loaded in cuts usr all signed and modified control file
		#lstBundleFiles.append(FileInfo(strSourceFolder + "sec\\dl.sec-scapp-1.5.11-80.tar", 1, 1, "", ""))	
		
#----------------------------------------------------------------------------------------------
		
def setVOSBaseFilesList():
	global strADKPath
	global strOutputDir

	if(strPackType=="NOPACK" ):
		return
	
	strSourceFolder = strADKPath + "\\load\\"
	
	if(strPackType=="PACK1"):	
		lstBundleFiles.append(FileInfo("C:\\toolchains\\os\\vos2\\dl.vos2-prod-V200c-Ctls-release-30410400.tgz", 0, 1, "", ""))		# OS. Must go first
		
	if(strPackType=="PACK1" or strPackType=="PACK2"):
		lstBundleFiles.append(FileInfo(strSourceFolder + "solutions\\dl.adk_4.3.4-369_vos_sys-all.tgz", 0, 1, "", ""))
		lstBundleFiles.append(FileInfo(strSourceFolder + "solutions\\dl.adk_4.3.4-369_vos_usr-all.tgz", 1, 1, "", ""))		
		#lstBundleFiles.append(FileInfo(strSourceFolder + "sysinfo\\dl.mac-remove-3.26.3-prod.tgz", 0, 1, "", ""))
		
#----------------------------------------------------------------------------------------------
		
def setEVOBaseFilesList():
	global strADKPath
	global strOutputDir
	
	if(strPackType=="NOPACK" ):
		return
	
	strSourceFolder = strADKPath + "\\load\\"
	
	if(strPackType=="PACK1"):	
		lstBundleFiles.append(FileInfo("C:\\toolchains\\os\\evo\\WT000500-20160729.zip", 0, 0, "1", "I"))		# OS. Must go first
		lstBundleFiles.append(FileInfo("C:\\toolchains\\os\\evo\\Vx011-EOS.zip", 0, 0, "1", "I"))
		
	if(strPackType=="PACK1" or strPackType=="PACK2"):
		lstBundleFiles.append(FileInfo(strSourceFolder + "emv\\dl.libEMV_CTLS_Client.so.11.30.0+CS.zip", 1, 1, "1", "I"))
		lstBundleFiles.append(FileInfo(strSourceFolder + "emv\\dl.libEMV_CT_Client.so.6.30.0+CS.zip", 1, 1, "1", "I"))
		lstBundleFiles.append(FileInfo(strSourceFolder + "emv\\dl.libEMV_CT_Client.so.6.30.0.zip", 1, 1, "1", "I"))
		lstBundleFiles.append(FileInfo(strSourceFolder + "emv\\dl.libEMV_CT_Framework.so.3.30.0+VEL.zip", 1, 1, "1", "I"))
		lstBundleFiles.append(FileInfo(strSourceFolder + "emv\\dl.libEMV_CT_VelocityK.so.2.1.4+VEL7.0.1.zip", 1, 1, "1", "I"))
		lstBundleFiles.append(FileInfo(strSourceFolder + "com\\dl.com-2.7.7-176.zip", 1, 1, "1", "I"))
		lstBundleFiles.append(FileInfo(strSourceFolder + "posix\\dl.libposix-1.7.3-1.zip", 1, 1, "1", "I"))
		lstBundleFiles.append(FileInfo(strSourceFolder + "inf\\dl.inf-1.11.3.zip", 1, 1, "1", "I"))
		lstBundleFiles.append(FileInfo(strSourceFolder + "guiprt\\dl.libcpapp-2.12.1-1.zip", 1	, 1, "1", "I"))
		lstBundleFiles.append(FileInfo(strSourceFolder + "guiprt\\dl.guiprtserver-2.12.1-1.zip", 1, 1, "1", "I"))
		lstBundleFiles.append(FileInfo(strSourceFolder + "ipc\\dl.libvfiipc-1.7.2-1.zip", 1, 1, "1", "I"))
		lstBundleFiles.append(FileInfo(strSourceFolder + "sqlite\\dl.sqlite-1.2.9.zip", 1, 1, "1", "I"))
		lstBundleFiles.append(FileInfo(strSourceFolder + "expat\\dl.expat-1.0.7.zip", 1, 1, "1", "I"))
		lstBundleFiles.append(FileInfo(strSourceFolder + "sysinfo\\dl.libsysbar-3.20.0.zip", 1, 1, "1", "I"))
		lstBundleFiles.append(FileInfo(strSourceFolder + "sysinfo\\dl.libsysinfo-3.20.0.zip", 1, 1, "1", "I"))
		lstBundleFiles.append(FileInfo(strSourceFolder + "sysinfo\\dl.asl-0.6.0.zip", 1, 1, "1", "I"))
		lstBundleFiles.append(FileInfo(strSourceFolder + "evt\\dl.libevt-2.3.5.zip", 1, 1, "1", "I"))
		lstBundleFiles.append(FileInfo(strSourceFolder + "sec\\dl.sec-1.5.9-76.zip", 1, 1, "1", "I"))
		lstBundleFiles.append(FileInfo(strSourceFolder + "log\\dl.liblog-2.2.6.zip", 1, 1, "1", "I"))
		lstBundleFiles.append(FileInfo(strSourceFolder + "msr\\dl.libmsr-2.4.0-26.zip", 1, 1, "1", "I"))
		lstBundleFiles.append(FileInfo(strSourceFolder + "guiprt\\dl.libvfiguiprt-2.12.1-1.zip", 1, 1, "1", "I"))
		lstBundleFiles.append(FileInfo(strSourceFolder + "guiprt\\dl.avcodec-1.0.0.9.zip", 1, 1, "1", "I"))
		#lstBundleFiles.append(FileInfo(strSourceFolder + "guiprt\\dl.libjsproc-2.12.1-1.zip", 1, 1, "1", "I"))
		lstBundleFiles.append(FileInfo(strSourceFolder + "tec\\dl.libtec-2.3.4-30.zip", 1, 1, "1", "I"))
		lstBundleFiles.append(FileInfo(strSourceFolder + "log\\dl.syslog-2.2.6.zip", 1, 1, "1", "I"))
		
	if(strPackType=="PACK3"):
		lstBundleFiles.append(FileInfo(strSourceFolder + "guiprt\\dl.guiprtserver-2.12.1-1.zip", 1, 1, "1", "I"))
		
		

		
#----------------------------------------------------------------------------------------------
		
def emptyFolder(folder, extension):
	for dirpath, dirnames, filenames in os.walk(folder):
		for filename in filenames:
			if(extension):
				if filename.endswith("."+extension):
					os.unlink(os.path.join(dirpath, filename))
			else:
				os.unlink(os.path.join(dirpath, filename))

#----------------------------------------------------------------------------------------------
				
def printUsageInfo():
	print("USAGE:")
	print("    python.exe build.by (APPNAME) (VERSION) (PLATFORM) (PACKTYPE)")
	print("    PLATFORM : EVO / VOS / VOS2")
	print("    PACKTYPE : PACK1 (OS+ADK+APP")
	print("               PACK2 (ADK+APP")
	print("               PACK3 (APP)")
	print("    COMPILE : (YES/NO)")
	


#----------------------------------------------------------------------------------------------
	
def packVOSVOS2():

	print("> Packing base software...")

	if(strPackType=="NOPACK" ):
		return
	
	for i in range(len(lstBundleFiles)):
		os.chdir(strOutputDir)	
		emptyFolder(strTempDir, "")
		if(lstBundleFiles[i].extractTGZ==0):
			print ("> Copying file ... " + lstBundleFiles[i].name)
			shutil.copy(lstBundleFiles[i].name, strTempDir3)
		else:
			print ("> Extracting file ..." + lstBundleFiles[i].name)
			#os.system(strTarExePath + " --extract --file=" + lstBundleFiles[i].name + " -C ./temp1 --no-anchored --force-local --wildcards '*.tgz'")
			os.system("C:\\PROGRA~1\\7-Zip\\7z.exe x " +  lstBundleFiles[i].name + " -so | C:\\PROGRA~1\\7-Zip\\7z.exe x -aoa -si -ttar -o.\\temp1 ")
			#os.system("pause")
			if(lstBundleFiles[i].signed==0):
				#os.system(strTarExePath + " --extract --file=" + lstBundleFiles[i].name + " -C ./temp1 --no-anchored --force-local --wildcards '*.p7s'")
				os.system("C:\\PROGRA~1\\7-Zip\\7z.exe x " +  lstBundleFiles[i].name + " -so | C:\\PROGRA~1\\7-Zip\\7z.exe x -aoa -si -ttar -o.\\temp1 ")
			else:
				for file in os.listdir(strTempDir):
					if file.endswith(".tgz"):
						#os.system(strTarExePath + " --extract --file=" + strTempDir + "\\" + file + " -C ./temp2 --no-anchored --force-local")
						os.system("C:\\PROGRA~1\\7-Zip\\7z.exe x " + strTempDir + "\\" + file  + " -so | C:\\PROGRA~1\\7-Zip\\7z.exe x -aoa -si -ttar -o.\\temp2")
						emptyFolder(strTempDir2, "p7s")
						emptyFolder(strTempDir2, "tgz")
						for fileTAR in os.listdir(strTempDir2):
							if fileTAR.endswith(".tar"):
								os.system("FileSignature.exe -C " + strTempDir2 + "\\crt\\Certif.crt -F " + strTempDir2 + "\\" + fileTAR + " -SM")
						os.remove(strTempDir + "\\" + file)
						os.chdir(strTempDir2)
						print("C:\\PROGRA~1\\7-Zip\\7z.exe" + " a -ttar -i!* -so | C:\\PROGRA~1\\7-Zip\\7z.exe a -si -tgzip" + strTempDir + "\\" + file)
						#os.system("C:\\PROGRA~1\\7-Zip\\7z.exe" + " a -ttar -so " + "cert_pkg-1.0.tar | C:\\PROGRA~1\\7-Zip\\7z.exe a -si -tgzip " + strTempDir + "\\" + file )						
						os.system("C:\\PROGRA~1\\7-Zip\\7z.exe" + " a -ttar -so " + file +" | C:\\PROGRA~1\\7-Zip\\7z.exe a -si -tgzip " + strTempDir + "\\" + file )
						#os.system(strTarExePath + " -czf " + strTempDir + "\\" + file + " * --no-anchored --force-local --mode=755")
						os.system("FileSignature.exe -C " + strOutputDir + "\\crt\\Certif.crt -F " + strTempDir + "\\" + file + " -SM")			
						emptyFolder(strTempDir2,"")
						os.chdir(strOutputDir)
						
		for file in os.listdir(strTempDir):
			if(os.path.exists(strTempDir3 + "\\" + file)):
				os.remove(strTempDir3 + "\\" + file)
			shutil.move(strTempDir + "\\" + file, strTempDir3)

	print("-------------------------------------------------------------------------------")	
	print("> Packing Application...")

	#Create outter CONTROL info --------------------------------

	emptyFolder(strTempDir, "")
	os.chdir(strTempDir)

	if os.path.exists(".\\CONTROL"):
		wraprmtree(".\\CONTROL")

	if os.path.exists(".\\CONTROL"):
		wraprmtree(".\\CONTROL")
	os.makedirs(".\\CONTROL")
	os.chdir(".\\CONTROL")

	fileObj = open("control", "w")
	fileObj.write("Package: " + strAppName + "\n")
	fileObj.write("User: usr1\n")
	fileObj.write("Version: " + strAppVersion + "\n")
	fileObj.close()

	fileObj = open("remove", "w")
	fileObj.write("removeresources\n")
	fileObj.write("removefonts\n")
	fileObj.close()

	#Create fonts package --------------------------------------

	#os.chdir(strOutputDir)
	#wraprmtree(strTempDir2)
	#distutils.dir_util._path_created = {}
	#os.makedirs(strTempDir2)
	#os.chdir(strTempDir2)

	#distutils.dir_util._path_created = {}
	#copy_tree(strResourcesFolder + "\\fonts", ".")
	#os.system(strTarExePath + " -cf " + "pkg.dl.fonts.tar" + " * --mode=755")		
	#os.system("FileSignature.exe -C " + strOutputDir + "\\crt\\Certif.crt -F " + strTempDir2 + "\\" + "pkg.dl.fonts.tar" + " -SM")		

	#shutil.move(strTempDir2 + "\\" + "pkg.dl.fonts.tar", strTempDir)
	#shutil.move(strTempDir2 + "\\" + "pkg.dl.fonts.tar.p7s", strTempDir)


	#Create resources package ----------------------------------

	os.chdir(strOutputDir)
	wraprmtree(strTempDir2)
	distutils.dir_util._path_created = {}
	os.makedirs(strTempDir2)
	os.chdir(strTempDir2)

	distutils.dir_util._path_created = {}
	copy_tree(strResourcesFolder + "\\www", ".")
	os.system(strTarExePath + " -cf " + "pkg.dl.resources.tar" + " * --mode=755")		
	os.system("FileSignature.exe -C " + strOutputDir + "\\crt\\Certif.crt -F " + strTempDir2 + "\\" + "pkg.dl.resources.tar" + " -SM")		

	shutil.move(strTempDir2 + "\\" + "pkg.dl.resources.tar", strTempDir)
	shutil.move(strTempDir2 + "\\" + "pkg.dl.resources.tar.p7s", strTempDir)

	#Create pwr mng package ----------------------------------

	os.chdir(strOutputDir)
	wraprmtree(strTempDir2)
	distutils.dir_util._path_created = {}
	os.makedirs(strTempDir2)
	os.chdir(strTempDir2)

	distutils.dir_util._path_created = {}
	copy_tree(strResourcesFolder + "\\pwr", ".")
	os.system(strTarExePath + " -cf " + "pkg.dl.pwr.tar" + " * --mode=755")		
	os.system("FileSignature.exe -C " + strOutputDir + "\\crt\\Certif.crt -F " + strTempDir2 + "\\" + "pkg.dl.pwr.tar" + " -SM")		

	shutil.move(strTempDir2 + "\\" + "pkg.dl.pwr.tar", strTempDir)
	shutil.move(strTempDir2 + "\\" + "pkg.dl.pwr.tar.p7s", strTempDir)

	#VHQ CONFIG --------------------------------------------------
	
	#os.chdir(strOutputDir)
	#wraprmtree(strTempDir2)
	#distutils.dir_util._path_created = {}
	#os.makedirs(strTempDir2)
	#os.chdir(strTempDir2)

	#distutils.dir_util._path_created = {}
	#copy_tree(strResourcesFolder + "\\vhq\\config", ".")
	#os.system(strTarExePath + " -cf " + "pkg.dl.vhq.tar" + " * --mode=755")
	#os.system("FileSignature.exe -C " + strOutputDir + "\\crt\\Certif.crt -F " + strTempDir2 + "\\" + "pkg.dl.vhq.tar" + " -SM")

	#shutil.move(strTempDir2 + "\\" + "pkg.dl.vhq.tar", strTempDir)
	#shutil.move(strTempDir2 + "\\" + "pkg.dl.vhq.tar.p7s", strTempDir)
	
	#Database --------------------------------------------------
	#time.sleep(50.0/1000.0)
	#os.chdir(strOutputDir)
	#wraprmtree(strTempDir2)
	#distutils.dir_util._path_created = {}
	#os.makedirs(strTempDir2)
	#os.chdir(strTempDir2)

	#distutils.dir_util._path_created = {}
	#copy_tree(strResourcesFolder + "\\database", ".")
	#os.system(strTarExePath + " -cf " + "pkg.dl.database.tar" + " * --mode=755")
	#os.system("FileSignature.exe -C " + strOutputDir + "\\crt\\Certif.crt -F " + strTempDir2 + "\\" + "pkg.dl.database.tar" + " -SM")

	#shutil.move(strTempDir2 + "\\" + "pkg.dl.database.tar", strTempDir)
	#shutil.move(strTempDir2 + "\\" + "pkg.dl.database.tar.p7s", strTempDir)

	
	#App's binaries --------------------------------------------

	os.chdir(strOutputDir)
	wraprmtree(strTempDir2)
	distutils.dir_util._path_created = {}
	os.makedirs(strTempDir2)
	os.chdir(strTempDir2)


	if os.path.exists(".\\CONTROL"):
		wraprmtree(".\\CONTROL")

	if os.path.exists(".\\CONTROL"):
		wraprmtree(".\\CONTROL")
	os.makedirs(".\\CONTROL")
	os.chdir(".\\CONTROL")

	fileObj = open("control", "w")
	fileObj.write("Package: " + strAppName + "\n")
	fileObj.write("User: usr1\n")
	fileObj.write("Version: " + strAppVersion + "\n")
	fileObj.write("Group: system\n")
	fileObj.close()

	fileObj = open("start", "w")
	fileObj.write(strAppName + "\n")
	fileObj.close()

	os.chdir("..")

	#shutil.copy(strCurDir + "\\config\\scapp.cfg", ".")
	#shutil.copy(strCurDir + "\\config\\01.cfg", ".")
	#shutil.copy(strCurDir + "\\config\\scapp.cfg", ".")
	shutil.copy(strCurDir + "\\out\\" + strAppName, ".")
	#HARD CODE CAPX
	shutil.copy(strCurDir + "\\syslog.conf", ".")
	shutil.copy(strCurDir + "\\..\\resources\\config\\config.usr1", ".\\CONTROL")
	
	
	os.makedirs(".\\flash")
	shutil.copy(strCurDir + "\\..\\resources\\flash\\emv\\"+strProdDev+"\\EMV_Keys.xml", ".\\flash")
	shutil.copy(strCurDir + "\\..\\resources\\flash\\emv\\"+strProdDev+"\\EMV_CTLS_Keys.xml", ".\\flash")
	shutil.copy(strCurDir + "\\..\\resources\\flash\\emv\\emv-desired.xml", ".\\flash")
	shutil.copy(strCurDir + "\\..\\resources\\flash\\emv\\EMV_Applications.xml", ".\\flash")
	shutil.copy(strCurDir + "\\..\\resources\\flash\\emv\\EMV_Terminal.xml", ".\\flash")
	shutil.copy(strCurDir + "\\..\\resources\\flash\\emv\\EMV_CTLS_Terminal.xml", ".\\flash")
	shutil.copy(strCurDir + "\\..\\resources\\flash\\emv\\EMV_CTLS_Applications.xml", ".\\flash")
	shutil.copy(strCurDir + "\\..\\resources\\flash\\emv\\EMV_CTLS_Apps_SchemeSpecific.xml", ".\\flash")

	#os.makedirs(".\\flash\\promoimg")
	#copy_tree(strCurDir + "\\..\\resources\\flash\\promoimg", ".\\flash\\promoimg\\")
	os.makedirs(".\\flash\\themes")
	copy_tree(strCurDir + "\\..\\resources\\flash\\themes", ".\\flash\\themes\\")
	os.makedirs(".\\flash\\net")
	copy_tree(strCurDir + "\\..\\resources\\flash\\net", ".\\flash\\net")
	os.makedirs(".\\flash\\Comandos_PinPad")
	copy_tree(strCurDir + "\\..\\resources\\flash\\Comandos_PinPad", ".\\flash\\Comandos_PinPad")

	os.makedirs(".\\lib")
	shutil.copy(strSDKLibPath + "\\svcmgr\\libsvc_logmgr.so", ".\\lib" )
	shutil.copy(strSDKLibPath + "\\svcmgr\\libsvc_usbg.so", ".\\lib" )
	shutil.copy(strSDKLibPath + "\\svcmgr\\libsvc_sound.so", ".\\lib" )
	shutil.copy(strSDKPath + "\\usr\\lib\\libasound.so", ".\\lib" )
	#shutil.copy(strSDKLibPath + "\\liblogapi.so", ".\\lib" )
	#shutil.copy(strSDKLibPath + "\\libvfisec.so", ".\\lib" )
	shutil.copy(strSDKLibPath + "\\svcmgr\\libsvc_security.so", ".\\lib" )
	#ADK libs -------------------------------------------------
	#shutil.copy(strADKlibPath + "\\libsvc_tms.so", ".\\lib" )
	#USER Libs
	shutil.copy(strCurDir + "\\..\\lib\\libAdkCapX.so", ".\\lib" )
	shutil.copy(strCurDir + "\\..\\lib\\libAES_encryp.so", ".\\lib" )
	shutil.copy(strCurDir + "\\..\\lib\\libCmdPP.so", ".\\lib" )
	shutil.copy(strCurDir + "\\..\\lib\\libComRdc.so", ".\\lib" )
	shutil.copy(strCurDir + "\\..\\lib\\libVosEMVProcess.so", ".\\lib" )
	#shutil.copy(strCurDir + "\\..\\lib\\libvhqrdc.so", ".\\lib" )
	


	#Final touches ---------------------------------------------

	os.chdir(strTempDir2)
	strTarFileName = "pkg." + strAppName + ".tar"
	copy_tree(strOutputDir + "\\crt", strTempDir2 + "\\crt")
	os.system(strTarExePath + " -cf " + strTarFileName + " * --force-local --exclude=" + strTarFileName + " --mode=755")			
	os.system("FileSignature.exe -C " + strOutputDir + "\\crt\\Certif.crt -F " + strTempDir2 + "\\" + strTarFileName + " -SM")		
	shutil.move(strTempDir2 + "\\" + strTarFileName, strTempDir)
	shutil.move(strTempDir2 + "\\" + strTarFileName + ".p7s", strTempDir)

	distutils.dir_util._path_created = {}
	copy_tree(strOutputDir + "\\crt", strTempDir + "\\crt")

	os.chdir(strTempDir)
	strTarFileName = "usr1.bundle.dl." + strAppName + ".tgz"
	#os.system(strTarExePath + " -czf " + strTarFileName + " * --force-local --exclude=" + strTarFileName + " --mode=755")			
	os.system("C:\\PROGRA~1\\7-Zip\\7z.exe" + " a -ttar -so " + "usr1.bundle.dl." + strAppName +".tar | C:\\PROGRA~1\\7-Zip\\7z.exe a -si -tgzip " + strTarFileName )
	os.system("FileSignature.exe -C " + strOutputDir + "\\crt\\Certif.crt -F " + strTempDir + "\\" + strTarFileName + " -SM")		

	shutil.move(strTarFileName, strTempDir3)
	shutil.move(strTarFileName + ".p7s", strTempDir3)

	os.chdir(strTempDir3)

	#remove mac we do not use
	if os.path.exists("mac.tgz"):
		os.remove("mac.tgz")
		os.remove("mac.tgz.p7s")

	strTarFileName = "dl." + strAppName + "_"+ strAppVersion +"_"+ strProdDev + ".tar"
	os.system(strTarExePath + " -cf " + strTarFileName + " * --force-local --exclude=" + strTarFileName + " --mode=755")			

	shutil.move(strTarFileName, strOutputDir)
	
	
#----------------------------------------------------------------------------------------------
	

				
# MAIN PROGRAM ################################################################################


# Check Arguments -----------------------------------------------------------------------------

if(not(len(sys.argv))==7):
	printUsageInfo()
	quit()

strAppName = sys.argv[1];	
strAppVersion = sys.argv[2];
strPlatform = sys.argv[3];
strPackType = sys.argv[4];
strCompile = sys.argv[5];
strProdDev = sys.argv[6];

if(not(strPlatform=="EVO" or strPlatform=="VOS" or strPlatform=="VOS2")):
	print("ERROR: Invalid Plaform!")
	printUsageInfo()
	quit()
	
if(not(strPackType=="PACK1" or strPackType=="PACK2" or strPackType=="PACK3" or strPackType=="NOPACK" )):
	print("ERROR:  Invalid Pack Option!")
	printUsageInfo()
	quit()
	
# Main body -----------------------------------------------------------------------------------

print("###############################################################################")
print("App       : " + strAppName)
print("Platform  : " + strPlatform)
print("PROD/DEV  : " + strProdDev)
from datetime import datetime
print("Start Time: " + datetime.now().strftime('%H:%M:%S'))
print("-------------------------------------------------------------------------------")

if(strPlatform=="VOS"):
	setVOSConfig()
	
if(strPlatform=="VOS2"):
	setVOS2Config()	

if(strPlatform=="EVO"):
	setEVOConfig()	

print("ToolChain : " + strToolChainPath)
print("Cygwin    : " + srtCygwinTool)
print("SDK       : " + strSDKPath)
print("ADK       : " + strADKPath)
#print("Compiler  : " + strCompilerPath)
#print("Linker    : " + strLinkerPath)
print("-------------------------------------------------------------------------------")

print("> Setting enviroment vars...")
os.environ["APP_NAME"] = strAppName
os.environ["PLATFORM"] = strPlatform
os.environ["TOOLCHAIN_PATH"] = strToolChainPath
os.environ["SDK_PATH"] = strSDKPath
os.environ["SDK_LIB_PATH"] = strSDKLibPath
os.environ["SDK_INCLUDES"] = strSDKIncludes
os.environ["EOS_PATH"] = strEOSPath
os.environ["EOS_INCLUDES"] = strEOSIncludes
os.environ["ADK_PATH"] = strADKPath
os.environ["COMPILER"] = strCompilerPath
os.environ["LINKER"] = strLinkerPath
os.environ["COPTIONS"] = strCompilerOptions
os.environ["LOPTIONS"] = strLinkerOptions
os.environ["VRXSDK"] = strSDKPath		# Used by linker (EVO)
os.environ["CTLS_LIB_PATH"] = strCTLSLibPath
os.environ["ADK_LIB_PATH"] = strADKlibPath

print("> Creating required folders...")
createFoldersStructure()


print("> Compiling/Linking...")

if(strPlatform=="EVO"):
	if(strCompile != "NO"):
		os.system("C:\\Progra~2\\GnuWin32\\bin\\make.exe -j5 --file " + strAppName + ".evo.mk")
	#strTarExePath = "c:\\toolchains\\tools\\7z.exe"
	strTarExePath = "C:\\PROGRA~1\\7-Zip\\7z.exe"
else:
	if(strCompile != "NO"):
		print("Buscando make.exe")
		os.system("C:\\GnuWin32\\bin\\make.exe -j5 --file " + strAppName + ".mk")
	#strTarExePath = "c:\\toolchains\\tools\\tar.exe"
	#strTarExePath = srtCygwinTool + "\\tar.exe"
	#strTarExePath =  "tar.exe"
	print("Buscando tar.exe")
	#strTarExePath = "C:\\Progra~2\\VeriFone\\PackageManagerProduction\\Cygwin\\tar.exe"
	strTarExePath = "C:\\Cygwin\\bin\\tar.exe"
	
	
print("-------------------------------------------------------------------------------")	
print("-------------------------------------------------------------------------------")
print("End Time make:   " + datetime.now().strftime('%H:%M:%S'))


strTempDir = strOutputDir + "\\temp1"
strTempDir2 = strOutputDir + "\\temp2"
strTempDir3 = strOutputDir + "\\temp3"
strPkgName = "pkg." + strAppName  + ".tgz"
strBundleName = "usr1.bundle.dl." + strPkgName

if(strPlatform=="VOS"):
	setVOSBaseFilesList()
	packVOSVOS2()
elif(strPlatform=="VOS2"):
	setVOS2BaseFilesList()
	packVOSVOS2()

print("-------------------------------------------------------------------------------")
print("End Time Package:   " + datetime.now().strftime('%H:%M:%S'))


if(not strPackType=="NOPACK" ):		
	print("RUTA :" + strOutputDir + "\\" +  "dl." + strAppName + ".tar")
	time.sleep(1)
	os.chdir(strOutputDir)
	print(os.path.isfile("dl." + strAppName + ".tar"));
	if os.path.isfile("dl." + strAppName + ".tar"):
		print("Sending... ")
		os.system(strNetDown + "\\Mx800Downloader.exe -n " + strOutputDir + "\\" + "dl." + strAppName + ".tar" + " -send -run");

if(not strPackType=="NOPACK" ):		
	wraprmtree(strTempDir)	
	wraprmtree(strTempDir2)
	wraprmtree(strTempDir3)	
	wraprmtree(strOutputDir + "\\crt")

print("-------------------------------------------------------------------------------")
print("End Time:   " + datetime.now().strftime('%H:%M:%S'))
