@echo off



rem Sign APK
rem   arg1 : .apk file path
rem   arg2 : .keystore file path
rem   arg3 : .keystore password



if "%1" == "" (
    set /p TARGET_APK="TARGET APK = "
) else (
    set TARGET_APK=%1
)

if "%2" == "" (
    set /p KEYSTORE="KEYSTORE = "
) else (
    set KEYSTORE=%2
)

if "%3" == "" (
    set /p KEYPASS="KEYPASS = "
) else (
    set KEYPASS=%3
)



echo "###### SIGN TARGET APK #####################################"
call jarsigner -verbose -keystore %KEYSTORE% -storepass %KEYPASS% %TARGET_APK% key
