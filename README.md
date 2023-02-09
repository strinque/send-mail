# send-mail 📧

![send-mail help](https://github.com/strinque/send-mail/blob/master/docs/help.png)

The `send-mail` **Windows** program sends emails using the **smtp** protocol. 
Emails are sent using a **smtp**/**smtps** server with or without *username*/*password* command-line options.

Implemented in c++17 and use `vcpkg`/`cmake` for the build-system.  

It uses the `winpp` header-only library from: https://github.com/strinque/winpp.

## Features

- [x] use the open-source `mailio` library to format/send email
- [x] use the open-source `openssl` library for tls
- [x] check emails validity (src/dst/reply) using std::regex
- [x] can send emails to multiple destinations
- [x] can attach file to emails
- [x] provide a c++ class: `SendMail` to send email with arguments checked at compile-time
- [x] implement a program: `send-mail.exe` which use the `SendMail` to send emails

## Usage

``` console
send-email.exe --src-name \"src name\" \
               --src-email src@xxx.com \
               --dst-name \"dstName A;dstName B;\" \
               --dst-email \"dstA@yyy.com; dstB@yyy.com;" \
               --smtp-server xxx.smtp.com \
               --smtp-username user \
               --smtp-password password \
               --smtp-tls \
               --email-title "Title" \
               --email-content "Content of the mail\r\nWith newlines" \
               --email-file "file.7z"
```

The following arguments are mandatory for `send-email.exe`:

 - --smtp-server
 - --src-email
 - --dst-email
 - --email-title
 - --email-content

## SendMail

The `SendMail` class contains all the logic for formatting/sending the emails.  
The interface is simple, all arguments are given to the Email class constructor in any order.  
The types of these arguments will be checked during compile-time thanks to templates.

The following arguments are mandatory for `SendMail`:

 - option::smtp_server: std::string
 - option::src_email: std::string
 - option::dst_email: std::string
 - option::email_title: std::string
 - option::email_content: std::string

These arguments have specific types:
 - option::email_file: std::filesystem::path
 - option::smtp_tls: bool

``` cpp
// construct email
Email mail({
  option::smtp_server("server.smtp.fr"),
  option::src_email("src@email.fr"),
  option::dst_email("dst@email.fr"),
  option::email_title("email title"),
  option::email_content("email content\r\nwith multiple lines")
  });

// send email
std::string mailio_error;
if (!mail.send(mailio_error))
  throw std::runtime_error(mailio_error);
```

## Requirements

This project uses **vcpkg**, a free C/C++ package manager for acquiring and managing libraries to build all the required libraries.  
It also needs the installation of the **winpp**, a private *header-only library*, inside **vcpkg**.

### Install vcpkg

The install procedure can be found in: https://vcpkg.io/en/getting-started.html.  
The following procedure installs `vcpkg` and integrates it in **Visual Studio**.

Open PowerShell: 

``` console
cd c:\
git clone https://github.com/Microsoft/vcpkg.git
cd vcpkg
.\bootstrap-vcpkg.bat
.\vcpkg.exe integrate install
```

Create a `x64-windows-static-md` triplet file used to build the program in *shared-mode* for **Windows CRT** libraries but *static-mode* for third-party libraries:

``` console
$VCPKG_DIR = Get-Content "$Env:LocalAppData/vcpkg/vcpkg.path.txt" -Raw 

Set-Content "$VCPKG_DIR/triplets/community/x64-windows-static-md.cmake" 'set(VCPKG_TARGET_ARCHITECTURE x64)
set(VCPKG_CRT_LINKAGE dynamic)
set(VCPKG_LIBRARY_LINKAGE static)
set(VCPKG_BUILD_TYPE release)'
```

### Install winpp ports-files

Copy the *vcpkg ports files* from **winpp** *header-only library* repository to the **vcpkg** directory.

``` console
$VCPKG_DIR = Get-Content "$Env:LocalAppData/vcpkg/vcpkg.path.txt" -Raw 

mkdir $VCPKG_DIR/ports/winpp
Invoke-WebRequest -Uri "https://raw.githubusercontent.com/strinque/winpp/master/vcpkg/ports/winpp/portfile.cmake" -OutFile "$VCPKG_DIR/ports/winpp/portfile.cmake"
Invoke-WebRequest -Uri "https://raw.githubusercontent.com/strinque/winpp/master/vcpkg/ports/winpp/vcpkg.json" -OutFile "$VCPKG_DIR/ports/winpp/vcpkg.json"
```

## Build

### Build using cmake

To build the program with `vcpkg` and `cmake`, follow these steps:

``` console
$VCPKG_DIR = Get-Content "$Env:LocalAppData/vcpkg/vcpkg.path.txt" -Raw 

git clone https://github.com/strinque/send-mail
cd send-mail
mkdir build; cd build
cmake -DCMAKE_BUILD_TYPE="MinSizeRel" `
      -DVCPKG_TARGET_TRIPLET="x64-windows-static-md" `
      -DCMAKE_TOOLCHAIN_FILE="$VCPKG_DIR/scripts/buildsystems/vcpkg.cmake" `
      ../
cmake --build . --config MinSizeRel
```

The program executable should be compiled in: `send-mail\build\src\MinSizeRel\send-mail.exe`.

### Build with Visual Studio

**Microsoft Visual Studio** can automatically install required **vcpkg** libraries and build the program thanks to the pre-configured files: 

- `CMakeSettings.json`: debug and release settings
- `vcpkg.json`: libraries dependencies

The following steps needs to be executed in order to build/debug the program:

``` console
File => Open => Folder...
  Choose send-mail root directory
Solution Explorer => Switch between solutions and available views => CMake Targets View
Select x64-release or x64-debug
Select the src\send-mail.exe (not bin\send-mail.exe)
```

To add command-line arguments for debugging the program:

```
Solution Explorer => Project => (executable) => Debug and Launch Settings => src\program.exe
```

``` json
  "args": [
    "--src-email src@xxx.com",
    "--dst-email dst@yyy.com",
    "--smtp-server xxx.smtp.com",
    "--smtp-username user",
    "--smtp-password password",
    "--smtp-tls",
    "--email-title \"Title\"",
    "--email-content \"Content of the mail\"",
    "--email-file \"file.7z\""
  ]
```