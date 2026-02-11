/*

    smtpClient.hpp 
 
    This file is part of Multitasking HTTP, FTP, Telnet, NTP, SMTP servers and clients for ESP32 - Arduino library: https://github.com/BojanJurca/Multitasking-Http-Ftp-Telnet-Ntp-Smtp-Servers-and-clients-for-ESP32-Arduino-Library

  
    February 6, 2026, Bojan Jurca

*/


#include "smtpClient.h"

  // sends message, returns error or success text (from SMTP server)
  Cstring<300> sendMail (const char *message, const char *subject, const char *to, const char *from, const char *password, const char *userName, int smtpPort, const char *smtpServer) {

      tcpClient_t smtpClient (smtpServer, smtpPort);
      if (smtpClient.errText ())
          return smtpClient.errText ();

      smtpClient.setIdleTimeout (SMTP_TIME_OUT);

      // connection is established now and the socket os opened

      Cstring<300> buffer;

      // 1. read welcome message from SMTP server
      switch (smtpClient.recvString (buffer, 300, "\n")) {
          case -1:  return strerror (errno);
          case 0:   return "Connection closed by peer";
          case 300: return "Buffer too small";
          default:  break; // continue
      }
      if (strstr (buffer, "220") != buffer) // 220 mail.siol.net ESMTP server ready
          return buffer;

      // 2. send EHLO to SMTP server (the buffer should have enough capacity)
      if (sizeof (buffer) < 5 + strlen (HOSTNAME) + 3)
          return "Buffer too small";
      sprintf (buffer, "EHLO %s\r\n", HOSTNAME);
      switch (smtpClient.sendString (buffer)) {
          case -1:  return strerror (errno);
          case 0:   return "Connection closed by peer";
          default:  break;
      }

      // 3. get the reply from SMTP server
      switch (smtpClient.recvString (buffer, 300, "\n")) {
          case -1:  return strerror (errno);
          case 0:   return "Connection closed by peer";            
          case 300: return "Buffer too small";
          default:  break; // continue
      }
      if (strstr (buffer, "250") != buffer) // 250-mail.siol.net ...
          return buffer;

      // 4. send login request to SMTP server
      switch (smtpClient.sendString ("AUTH LOGIN\r\n")) {
          case -1:  return strerror (errno);
          case 0:   return "Connection closed by peer";
          default:  break;
      }

      // 5. get the reply from SMTP server
      switch (smtpClient.recvString (buffer, 300, "\n")) {
          case -1:  return strerror (errno);
          case 0:   return "Connection closed by peer"; 
          case 300: return "Buffer too small";
          default:  break; // continue
      }
      if (strstr (buffer, "334") != buffer) // 334 VXNlcm5hbWU6 (= base64 encoded "Username:")
          return buffer;

      // 6. send base64 encoded user name to SMTP server
      size_t encodedLen;
      mbedtls_base64_encode ((unsigned char *) buffer.c_str (), buffer.max_size () - 3, &encodedLen, (const unsigned char *) userName, strlen (userName));
      strcpy (buffer + encodedLen, "\r\n");
      switch (smtpClient.sendString (buffer)) {
          case -1:  return strerror (errno);
          case 0:   return "Connection closed by peer";
          default:  break;
      }

      // 7. get the reply from SMTP server
      switch (smtpClient.recvString (buffer, 300, "\n")) {
          case -1:  return strerror (errno);
          case 0:   return "Connection closed by peer"; 
          case 300: return "Buffer too small";
          default:  break; // continue
      }
      if (strstr (buffer, "334") != buffer) // 334 UGFzc3dvcmQ6 (= base64 encoded "Password:")
          return buffer;

      // 8. send base64 encoded password to SMTP server
      mbedtls_base64_encode ((unsigned char *) buffer.c_str (), buffer.max_size () - 3, &encodedLen, (const unsigned char *) password, strlen (password));
      strcpy (buffer + encodedLen, "\r\n");
      switch (smtpClient.sendString (buffer)) {
          case -1:  return strerror (errno);
          case 0:   return "Connection closed by peer";
          default:  break;
      }

      // 9. get the reply from SMTP server
      switch (smtpClient.recvString (buffer, 300, "\n")) {
          case -1:  return strerror (errno);
          case 0:   return "Connection closed by peer";
          case 300: return "Buffer too small";
          default:  break; // continue
      }
      if (strstr (buffer, "235") != buffer) // 235 2.7.0 Authentication successful 
          return buffer;

      // 10. send MAIL FROM to SMTP server - there should be only one address in from string (there is onlyone sender) - parse it against @ chracter and get the reply(es)
      if (buffer.max_size () < 13 + strlen (from) + 3)
          return "Buffer too small";
      strcpy (buffer.c_str () + 13, from); // leave some space at the start of the buffer
      {
          bool quotation = false;
          for (int i = 13; buffer [i]; i++) {  
              switch (buffer [i]) {
                  case '\"':  quotation = !quotation; 
                              break;
                  case '@':   if (!quotation) { // we found @ in email address
                                  int j; for (j = i - 1; j > 0 && buffer [j] > '\"' && buffer [j] != ','; j--); // go back to the beginning of email address
                                  if (j == 0) j--;
                                  int k; for (k = i + 1; buffer [k] > '\"' && buffer [k] != ',' && buffer [k] != ';'; k++); buffer [k] = 0; // go forward to the end of email address
                                  i = k;
                                  // strcpy (buffer, "MAIL FROM:"); strcat (buffer, buffer + j + 1); strcat (buffer, "\r\n");
                                  buffer = Cstring<64> ("MAIL FROM:") + Cstring<300> (buffer.c_str () + j + 1) + "\r\n";

                                  // send the buffer now
                                  switch (smtpClient.sendString (buffer)) {
                                      case -1:  return strerror (errno);
                                      case 0:   return "Connection closed by peer";
                                      default:  break;
                                  }

                                  // get the reply from SMTP server
                                  switch (smtpClient.recvString (buffer, 300, "\n")) {
                                      case -1:  return strerror (errno);
                                      case 0:   return "Connection closed by peer";
                                      case 300: return "Buffer too small";
                                      default:  break; // continue
                                  }
                                  if (strstr (buffer, "250") != buffer) // 250 2.1.0 Ok
                                      return buffer;

                              }
                  default:    break;
              }
          }
      }

      // 11. send RCPT TO for each address in to-list, to SMTP server and read its reply - parse it against @ chracter and get the reply(es)
      if (sizeof (buffer) < 13 + strlen (to) + 1 + 1)
          return "Buffer too small";
      memset (buffer, 0, sizeof (buffer)); // fill the buffer with 0
      strcpy (buffer.c_str () + 13, to);
      {
          bool quotation = false;
          for (int i = 13; buffer [i]; i++) {  
              switch (buffer [i]) {
                  case '\"':  quotation = !quotation; 
                              break;
                  case '@':   if (!quotation) { // we found @ in email address
                                  int j; for (j = i - 1; j > 0 && buffer [j] > '\"' && buffer [j] != ','; j--); // go back to the beginning of email address
                                  if (j == 0) j--;
                                  int k; for (k = i + 1; buffer [k] > '\"' && buffer [k] != ',' && buffer [k] != ';'; k++); buffer [k] = 0; // go forward to the end of email address
                                  i = k;
                                  // strcpy (buffer, "RCPT TO:"); strcat (buffer, buffer + j + 1); strcat (buffer, "\r\n");
                                  buffer = Cstring<64> ("RCPT TO:") + Cstring<300> (buffer.c_str () + j + 1) + "\r\n";

                                  // send the buffer now
                                  switch (smtpClient.sendString (buffer)) {
                                      case -1:  return strerror (errno);
                                      case 0:   return "Connection closed by peer";
                                      default:  break;
                                  }

                                  // get the reply from SMTP server
                                  switch (smtpClient.recvString (buffer, 300, "\n")) {
                                      case -1:  return strerror (errno);
                                      case 0:   return "Connection closed by peer";
                                      case 300: return "Buffer too small";
                                      default:  break; // continue
                                  }
                                  if (strstr (buffer, "250") != buffer) // 250 2.1.0 Ok
                                      return buffer;

                              }
                  default:    break;
              }
          }
      }

      // 12. send DATA command to SMTP server
      switch (smtpClient.sendString ("DATA\r\n")) {
          case -1:  return strerror (errno);
          case 0:   return "Connection closed by peer";
          default:  break;
      }

      // 13. get the reply from SMTP server
      switch (smtpClient.recvString (buffer, 300, "\n")) {
          case -1:  return strerror (errno);
          case 0:   return "Connection closed by peer";
          case 300: return "Buffer too small";
          default:  break; // continue
      }
      if (strstr (buffer, "354") != buffer) // 354 End data with <CR><LF>.<CR><LF>
          return buffer;

      // 14. send DATA content to SMTP server
      {
          String s ("");
          int errCount = 0;
                errCount += !s.concat ("From:");
                errCount += !s.concat (from);
                errCount += !s.concat ("\r\nTo:");
                errCount += !s.concat (to);
                errCount += !s.concat ("\r\n");

          #ifdef __TIME_FUNCTIONS__
              time_t now = time (NULL);
              if (now) { // if we know the time than add this information also
                  struct tm structNow;
                  gmtime_r (&now, &structNow);
                  char stringNow [128];
                  strftime (stringNow, sizeof (stringNow), "%a, %d %b %Y %H:%M:%S %Z", &structNow);
                      errCount += !s.concat ("Date:");
                      errCount += !s.concat (stringNow);
                      errCount += !s.concat ("\r\n");
              }
          #endif

                errCount += !s.concat ("Subject:");
                errCount += !s.concat (subject);
                errCount += !s.concat ("\r\nContent-Type: text/html; charset=\"utf8\"\r\n" // add this directive to enable HTML content of message
                                        "\r\n");
                errCount += !s.concat (message);
                errCount += !s.concat ("\r\n.\r\n"); // end-of-message mark
          if (errCount)
              return "Out of memory";

          switch (smtpClient.sendString (s.c_str ())) {
              case -1:  return strerror (errno);
              case 0:   return "Connection closed by peer";
              default:  break;
          }

          // 15. get the reply from SMTP server which also indicates the success of the whole procedure
          switch (smtpClient.recvString (buffer, 300, "\n")) {
              case -1:  return strerror (errno);
              case 0:   return "Connection closed by peer";
              case 300: return "Buffer too small";
              default:  break; // continue
          }

          return buffer; // whatever the SMTP server sent
      }
  }
