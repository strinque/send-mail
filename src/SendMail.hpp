#pragma once
#include <string>
#include <vector>
#include <regex>
#include <map>
#include <filesystem>
#include <variant>
#include <initializer_list>
#include <fstream>
#include <stdbool.h>
#include <fmt/format.h>
#include <winpp/utf8.hpp>
#include <mailio/message.hpp>
#include <mailio/smtp.hpp>

// check email validity
inline bool check_email(const std::string& email)
{
  std::regex r(R"(^[\w-\.]+@([\w-]+\.)+[\w-]{2,4}$)");
  std::smatch sm;
  return std::regex_search(email, sm, r);
}

namespace details
{
  // list of option id
  enum class option_id
  {
    smtp_server,
    smtp_username,
    smtp_password,
    smtp_tls,
    src_name,
    src_email,
    dst_name,
    dst_email,
    email_title,
    email_content,
    email_file
  };
  const std::vector<std::string> option_name = 
  {
    "smtp_server",
    "smtp_username",
    "smtp_password",
    "smtp_tls",
    "src_name",
    "src_email",
    "dst_name",
    "dst_email",
    "email_title",
    "email_content",
    "email_file"
  };

  // template used to check the data type validity
  template<option_id ID, typename T1>
  struct option_data
  {
    template<typename T2>
    option_data(T2 a) :
      id(ID),
      arg(a)
    {
      static_assert(std::is_convertible_v<T1, T2>, "invalid data type for this option");
    }
    template<>
    option_data(const char* a) :
      id(ID),
      arg(a)
    {
      static_assert(std::is_convertible_v<T1, std::string>, "invalid data type for this option");
    }
    option_id id;
    T1 arg;
  };
}

namespace option
{
  // define options and their data type (to check format type afterwards)
  using smtp_server = details::option_data<details::option_id::smtp_server, std::string>;
  using smtp_username = details::option_data<details::option_id::smtp_username, std::string>;
  using smtp_password = details::option_data<details::option_id::smtp_password, std::string>;
  using smtp_tls = details::option_data<details::option_id::smtp_tls, bool>;
  using src_name = details::option_data<details::option_id::src_name, std::string>;
  using src_email = details::option_data<details::option_id::src_email, std::string>;
  using dst_name = details::option_data<details::option_id::dst_name, std::string>;
  using dst_email = details::option_data<details::option_id::dst_email, std::string>;
  using email_title = details::option_data<details::option_id::email_title, std::string>;
  using email_content = details::option_data<details::option_id::email_content, std::string>;
  using email_file = details::option_data<details::option_id::email_file, std::filesystem::path>;
}

namespace details
{
  // variant which contains all the different options available
  using OptionsVal = std::variant<option::smtp_server, option::smtp_username, option::smtp_password,
                                  option::smtp_tls, option::src_name, option::src_email, option::dst_name,
                                  option::dst_email, option::email_title, option::email_content, option::email_file>;

  // variant which contains all the different options data types
  using OptionsType = std::variant<std::string, std::filesystem::path, bool>;

  // store all the different options
  class Options final
  {
  public:
    Options(const std::initializer_list<OptionsVal>& opts) :
      m_opts()
    {
      for (const auto& o : opts)
      {
        if (std::holds_alternative<option::smtp_server>(o))
          setArg(option_id::smtp_server, std::get<option::smtp_server>(o).arg);
        else if (std::holds_alternative<option::smtp_username>(o))
          setArg(option_id::smtp_username, std::get<option::smtp_username>(o).arg);
        else if (std::holds_alternative<option::smtp_password>(o))
          setArg(option_id::smtp_password, std::get<option::smtp_password>(o).arg);
        else if (std::holds_alternative<option::smtp_tls>(o))
          setArg(option_id::smtp_tls, std::get<option::smtp_tls>(o).arg);
        else if (std::holds_alternative<option::src_name>(o))
          setArg(option_id::src_name, std::get<option::src_name>(o).arg);
        else if (std::holds_alternative<option::src_email>(o))
          setArg(option_id::src_email, std::get<option::src_email>(o).arg);
        else if (std::holds_alternative<option::dst_name>(o))
          setArg(option_id::dst_name, std::get<option::dst_name>(o).arg);
        else if (std::holds_alternative<option::dst_email>(o))
          setArg(option_id::dst_email, std::get<option::dst_email>(o).arg);
        else if (std::holds_alternative<option::email_title>(o))
          setArg(option_id::email_title, std::get<option::email_title>(o).arg);
        else if (std::holds_alternative<option::email_content>(o))
          setArg(option_id::email_content, std::get<option::email_content>(o).arg);
        else if (std::holds_alternative<option::email_file>(o))
          setArg(option_id::email_file, std::get<option::email_file>(o).arg);
      }
    }
    ~Options() = default;

    // get the option value from the map
    template<typename T>
    const T getArg(const option_id id) const
    {
      const auto it = m_opts.find(id);
      if (it == m_opts.end() || !std::holds_alternative<T>(it->second))
        return {};
      return std::get<T>(it->second);
    }

    // check if the option has been set
    bool hasArg(const option_id id) const
    {
      return (m_opts.find(id) != m_opts.end());
    }

    // check if the options have been set
    bool hasArgs(const std::vector<option_id>& ids) const
    {
      return std::find_if(ids.begin(), ids.end(), [=](const option_id id) {
        return m_opts.find(id) == m_opts.end();
        }) == ids.end();
    }

  private:
    // set the option value in the map
    void setArg(const option_id id, OptionsType value)
    {
      m_opts[id] = value;
    }

  private:
    std::map<option_id, OptionsType> m_opts;
  };
}

// send an email
class Email final
{
public:
  // constructor/destructor
  Email(const std::initializer_list<details::OptionsVal>& opts) :
    m_options(opts)
  {
  }
  ~Email() = default;

  // send email using mailio
  bool send(std::string& mailio_error = std::string()) noexcept
  {
    try
    {
      // check that all mandatory options are set
      if (!m_options.hasArgs({ details::option_id::smtp_server,
                         details::option_id::src_email,
                         details::option_id::dst_email,
                         details::option_id::email_title,
                         details::option_id::email_content }))
      {
        throw std::runtime_error(fmt::format("missing at least one mandatory argument: \n{}{}{}{}{}",
          "  --smtp-server\n",
          "  --src-email\n",
          "  --dst-email\n",
          "  --email-title\n",
          "  --email-content\n"
          ));
      }

      // retrieve parameters
      const std::string& smtp_server = m_options.getArg<std::string>(details::option_id::smtp_server);
      const std::string& smtp_username = m_options.getArg<std::string>(details::option_id::smtp_username);
      const std::string& smtp_password = m_options.getArg<std::string>(details::option_id::smtp_password);
      const bool smtp_tls = m_options.getArg<bool>(details::option_id::smtp_tls);
      const std::string& src_name = utf8::from_utf8(m_options.getArg<std::string>(details::option_id::src_name));
      const std::string& src_email = m_options.getArg<std::string>(details::option_id::src_email);
      const std::string& dst_name = utf8::from_utf8(m_options.getArg<std::string>(details::option_id::dst_name));
      const std::string& dst_email = m_options.getArg<std::string>(details::option_id::dst_email);
      const std::string& email_title = m_options.getArg<std::string>(details::option_id::email_title);
      const std::string& email_content = m_options.getArg<std::string>(details::option_id::email_content);
      const std::filesystem::path& email_file = m_options.getArg<std::filesystem::path>(details::option_id::email_file);

      // check email validity
      if (!check_email(src_email))
        throw std::runtime_error(fmt::format("invalid src_email: \"{}\"", src_email));
      if (!check_email(dst_email))
        throw std::runtime_error(fmt::format("invalid dst-email: \"{}\"", dst_email));

      // construct mailio message
      mailio::message msg;
      msg.from(mailio::mail_address(src_name, src_email));
      msg.reply_address(mailio::mail_address(src_name, src_email));
      msg.add_recipient(mailio::mail_address(dst_name, dst_email));
      msg.subject(email_title);
      msg.content(email_content);

      // attach file if necessary
      std::ifstream file;
      if (!email_file.empty())
      {
        file.open(email_file, std::ios::binary);
        if (!file.good())
          throw std::runtime_error(fmt::format("can't attach file: \"{}\"", email_file.string()));
        msg.attach({std::make_tuple(std::ref(file), email_file.filename().string(), mailio::message::content_type_t())});
      }

      // send mailio message
      if (smtp_tls)
      {
        mailio::smtps conn(smtp_server, 587);
        conn.authenticate(smtp_username,
          smtp_password,
          (!smtp_username.empty() && !smtp_password.empty()) ?
          mailio::smtps::auth_method_t::START_TLS :
          mailio::smtps::auth_method_t::NONE);
        conn.submit(msg);
      }
      else
      {
        mailio::smtp conn(smtp_server, 25);
        conn.authenticate(smtp_username,
          smtp_password,
          (!smtp_username.empty() && !smtp_password.empty()) ?
          mailio::smtp::auth_method_t::LOGIN :
          mailio::smtp::auth_method_t::NONE);
        conn.submit(msg);
      }
      return true;
    }
    catch (const std::exception& ex)
    {
      mailio_error = ex.what();
      return false;
    }
  }

private:
  details::Options m_options;
};