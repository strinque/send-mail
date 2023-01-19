#include <string>
#include <vector>
#include <filesystem>
#include <functional>
#include <stdbool.h>
#include <fmt/core.h>
#include <fmt/format.h>
#include <fmt/color.h>
#include <winsock2.h>
#include <winpp/console.hpp>
#include <winpp/parser.hpp>
#include "SendMail.hpp"

/*============================================
| Declaration
==============================================*/
// program version
const std::string PROGRAM_NAME = "send-mail";
const std::string PROGRAM_VERSION = "1.0";

// default length in characters to align status 
constexpr std::size_t g_status_len = 50;

/*============================================
| Function definitions
==============================================*/
// lambda function to show colored tags
auto add_tag = [](const fmt::color color, const std::string& text) {
  fmt::print(fmt::format(fmt::fg(color) | fmt::emphasis::bold, "[{}]\n", text));
};

// execute a sequence of actions with tags
void exec(const std::string& str, std::function<void()> fct)
{
  fmt::print(fmt::format(fmt::emphasis::bold, "{:<" + std::to_string(g_status_len) + "}", str + ": "));
  try
  {
    fct();
    add_tag(fmt::color::green, "OK");
  }
  catch (const std::exception& ex)
  {
    add_tag(fmt::color::red, "KO");
    throw ex;
  }
}

int main(int argc, char** argv)
{
  // initialize Windows console
  console::init();

  // parse command-line arguments
  std::string src_name;
  std::string src_email;
  std::string dst_name;
  std::string dst_email;
  std::string smtp_server;
  std::string smtp_username;
  std::string smtp_password;
  bool smtp_tls = false;
  std::string email_title;
  std::string email_content;
  std::filesystem::path email_file;
  console::parser parser(PROGRAM_NAME, PROGRAM_VERSION);
  parser.add("n", "src-name", "name of the source address", src_name)
        .add("s", "src-email", "source of the email", src_email, true)
        .add("m", "dst-name", "name of the destination address", dst_name)
        .add("d", "dst-email", "destination of the email", dst_email, true)
        .add("x", "smtp-server", "smtp server address", smtp_server, true)
        .add("u", "smtp-username", "username for the smtp server", smtp_username)
        .add("p", "smtp-password", "password of the smtp server", smtp_password)
        .add("t", "smtp-tls", "activate tls for smtp server", smtp_tls)
        .add("e", "email-title", "set the email title", email_title, true)
        .add("c", "email-content", "set the email content", email_content, true)
        .add("f", "email-file", "attach a file to the email content", email_file);
  if (!parser.parse(argc, argv))
  {
    parser.print_usage();
    return -1;
  }

  try
  {
    // check arguments validity
    if (smtp_username.empty() && !smtp_password.empty() ||
        !smtp_username.empty() && smtp_password.empty())
      throw std::runtime_error("--smtp-username must be defined with --smtp-password");
    if (!std::filesystem::exists(email_file))
      throw std::runtime_error(fmt::format("invalid attached file: \"{}\"", email_file.string()));

    exec("sending email", [=]() {
      // construct email
      Email mail({
        option::smtp_server(smtp_server),
        option::smtp_username(smtp_username),
        option::smtp_password(smtp_password),
        option::smtp_tls(smtp_tls),
        option::src_name(src_name),
        option::src_email(src_email),
        option::dst_name(dst_name),
        option::dst_email(dst_email),
        option::email_title(email_title),
        option::email_content(email_content),
        option::email_file(email_file)
        });

      // send email
      std::string mailio_error;
      if (!mail.send(mailio_error))
        throw std::runtime_error(mailio_error);
      });

    return 0;
  }
  catch (const std::exception& ex)
  {
    fmt::print("{} {}\n",
      fmt::format(fmt::fg(fmt::color::red) | fmt::emphasis::bold, "error:"),
      ex.what());
    return -1;
  }
}