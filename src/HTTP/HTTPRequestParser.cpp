#include "HTTPRequestParser.hpp"
#include "HTTPMethod.hpp"

#include <optional>
#include <utility>

constexpr auto SP = ' ';
constexpr auto CR = '\r';
constexpr auto LF = '\n';
constexpr auto CRLF = "\r\n";

HTTPRequestParser::ParseResult HTTPRequestParser::Parse(HTTPRequest& req, const std::string& data)
{
  HTTPRequestParser parser(req, data);

  if(auto result = parser.ParseRequestLine(); result != ParseResult::Success)
  {
	return result;
  }

  if(!parser.ExpectCRLF())
  {
	return ParseResult::InvalidRequestLine;
  }

  parser.m_Index += 2;

  // NOTE: Currently shouldn't fail.
  (void)parser.ParseHeaders();

  if(auto result = parser.ParseBody(); result != ParseResult::Success)
  {
	return result;
  }

  return ParseResult::Success;
}

HTTPRequestParser::HTTPRequestParser(HTTPRequest& req, std::string data) : m_Request(req), m_Data(std::move(data))
{
}

HTTPRequestParser::ParseResult HTTPRequestParser::ParseRequestLine()
{
  // Request-Line   = Method SP Request-URI SP HTTP-Version CRLF
  // Method         = "GET" | "POST" | "PUT" | "DELETE" | "HEAD"
  // Request-URI    = "*" | absoluteURI | abs_path | authority
  // HTTP-Version   = "HTTP" "/" 1*DIGIT "." 1*DIGIT

  auto method = ConsumeUntil(SP);
  if(method.empty())
  {
	return ParseResult::InvalidMethod;
  }
  m_Index++;

  auto enumMethod = HTTPMethodFromString(method);

  auto uri = ConsumeUntil(SP);
  if(uri.empty())
  {
	return ParseResult::InvalidURI;
  }
  m_Index++;

  auto version = ConsumeUntil(CRLF);
  if(version.empty())
  {
	return ParseResult::InvalidVersion;
  }

  if(!ExpectCRLF())
  {
	return ParseResult::InvalidVersion;
  }

  m_Request.m_RequestLine = HTTPRequestLine(enumMethod, uri, version);

  return ParseResult::Success;
}

HTTPRequestParser::ParseResult HTTPRequestParser::ParseHeaders()
{
  auto result = HTTPHeaders::FromString(m_Data.substr(m_Index));

  auto header = result.first;
  auto length = result.second;

  m_Request.m_Headers = header;
  m_Index += length;

  return ParseResult::Success;
}

HTTPRequestParser::ParseResult HTTPRequestParser::ParseBody()
{
  // message-body = entity-body
  // 		    | <entity-body encoded as per Transfer-Encoding>

  if(!CheckBody())
  {
	// Ignore body.
	return ParseResult::Success;
  }

  if(!ExpectCRLF())
  {
	return ParseResult::InvalidBody;
  }

  // FIXME: There is some bug here the m_BodySize is one larger than m_Index + m_BodySize.
  // 		This is an issue with something that increments m_Index.
  m_Index += 2;

  m_Request.m_Body = m_Data.substr(m_Index, m_BodySize);

  return ParseResult::Success;
}

bool HTTPRequestParser::CheckBody()
{
  // 1. If the METHOD is HEAD, the server MUST NOT return a message-body in the response.
  if(m_Request.GetMethod() == HTTPMethod::HEAD)
  {
	return false;
  }

  // 2. Check for Content-Length header.
  if(m_Request.GetHeaders().Has("Content-Length"))
  {
	try
	{
	  auto contentLength = std::stoi(m_Request.GetHeaders().Get("Content-Length"));
	  m_BodySize = contentLength;
	  return m_Data.size() - m_Index >= contentLength;
	}
	catch(...)
	{
	  return false;
	}
  }

  // FIXME: 3. Check for Transfer-Encoding header.

  return false;
}

std::string HTTPRequestParser::ConsumeUntil(char c)
{
  // FIXME: Clean this and the std::string version up.

  std::string temp;

  while(m_Index < m_Data.size())
  {
	if(m_Data[m_Index] == c)
	{
	  return temp;
	}

	temp += m_Data[m_Index];
	++m_Index;
  }

  return temp;
}

std::string HTTPRequestParser::ConsumeUntil(const std::string& str)
{
  std::string temp;

  while(m_Index < m_Data.size())
  {
	if(m_Data[m_Index] == str[0])
	{
	  if(m_Data.substr(m_Index, str.size()) == str)
	  {
		return temp;
	  }
	}

	temp += m_Data[m_Index];
	++m_Index;
  }

  return temp;
}

bool HTTPRequestParser::ExpectCRLF()
{
  if(m_Index + 1 >= m_Data.size())
  {
	return false;
  }

  return m_Data[m_Index] == CR && m_Data[m_Index + 1] == LF;
}
