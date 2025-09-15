#include <Arduino_RouterBridge.h>

static const char iot[] = {
/* https://iot.arduino.cc:8885 */
"-----BEGIN CERTIFICATE-----\n"
"MIIB0DCCAXagAwIBAgIUb62eK/Vv1baaPAaY5DADBUbxB1owCgYIKoZIzj0EAwIw\n"
"RTELMAkGA1UEBhMCVVMxFzAVBgNVBAoTDkFyZHVpbm8gTExDIFVTMQswCQYDVQQL\n"
"EwJJVDEQMA4GA1UEAxMHQXJkdWlubzAgFw0yNTAxMTAxMDUzMjJaGA8yMDU1MDEw\n"
"MzEwNTMyMlowRTELMAkGA1UEBhMCVVMxFzAVBgNVBAoTDkFyZHVpbm8gTExDIFVT\n"
"MQswCQYDVQQLEwJJVDEQMA4GA1UEAxMHQXJkdWlubzBZMBMGByqGSM49AgEGCCqG\n"
"SM49AwEHA0IABKHhU2w1UhozDegrrFsSwY9QN7M+ZJug7icCNceNWhBF0Mr1UuyX\n"
"8pr/gcbieZc/0znG16HMa2GFcPY7rmIdccijQjBAMA8GA1UdEwEB/wQFMAMBAf8w\n"
"DgYDVR0PAQH/BAQDAgEGMB0GA1UdDgQWBBRCZSmE0ASI0cYD9AmzeOM7EijgPjAK\n"
"BggqhkjOPQQDAgNIADBFAiEAz6TLYP9eiVOr/cVU/11zwGofe/FoNe4p1BlzMl7G\n"
"VVACIG8tL3Ta2WbIOaUVpBL2gfLuI9WSW1sR++zXP+zFhmen\n"
"-----END CERTIFICATE-----\n"
};

static const char ubi[] = {
/* https://ubidefeo.com:443 */
"-----BEGIN CERTIFICATE-----\n"
"MIIF3jCCA8agAwIBAgIQAf1tMPyjylGoG7xkDjUDLTANBgkqhkiG9w0BAQwFADCB\n"
"iDELMAkGA1UEBhMCVVMxEzARBgNVBAgTCk5ldyBKZXJzZXkxFDASBgNVBAcTC0pl\n"
"cnNleSBDaXR5MR4wHAYDVQQKExVUaGUgVVNFUlRSVVNUIE5ldHdvcmsxLjAsBgNV\n"
"BAMTJVVTRVJUcnVzdCBSU0EgQ2VydGlmaWNhdGlvbiBBdXRob3JpdHkwHhcNMTAw\n"
"MjAxMDAwMDAwWhcNMzgwMTE4MjM1OTU5WjCBiDELMAkGA1UEBhMCVVMxEzARBgNV\n"
"BAgTCk5ldyBKZXJzZXkxFDASBgNVBAcTC0plcnNleSBDaXR5MR4wHAYDVQQKExVU\n"
"aGUgVVNFUlRSVVNUIE5ldHdvcmsxLjAsBgNVBAMTJVVTRVJUcnVzdCBSU0EgQ2Vy\n"
"dGlmaWNhdGlvbiBBdXRob3JpdHkwggIiMA0GCSqGSIb3DQEBAQUAA4ICDwAwggIK\n"
"AoICAQCAEmUXNg7D2wiz0KxXDXbtzSfTTK1Qg2HiqiBNCS1kCdzOiZ/MPans9s/B\n"
"3PHTsdZ7NygRK0faOca8Ohm0X6a9fZ2jY0K2dvKpOyuR+OJv0OwWIJAJPuLodMkY\n"
"tJHUYmTbf6MG8YgYapAiPLz+E/CHFHv25B+O1ORRxhFnRghRy4YUVD+8M/5+bJz/\n"
"Fp0YvVGONaanZshyZ9shZrHUm3gDwFA66Mzw3LyeTP6vBZY1H1dat//O+T23LLb2\n"
"VN3I5xI6Ta5MirdcmrS3ID3KfyI0rn47aGYBROcBTkZTmzNg95S+UzeQc0PzMsNT\n"
"79uq/nROacdrjGCT3sTHDN/hMq7MkztReJVni+49Vv4M0GkPGw/zJSZrM233bkf6\n"
"c0Plfg6lZrEpfDKEY1WJxA3Bk1QwGROs0303p+tdOmw1XNtB1xLaqUkL39iAigmT\n"
"Yo61Zs8liM2EuLE/pDkP2QKe6xJMlXzzawWpXhaDzLhn4ugTncxbgtNMs+1b/97l\n"
"c6wjOy0AvzVVdAlJ2ElYGn+SNuZRkg7zJn0cTRe8yexDJtC/QV9AqURE9JnnV4ee\n"
"UB9XVKg+/XRjL7FQZQnmWEIuQxpMtPAlR1n6BB6T1CZGSlCBst6+eLf8ZxXhyVeE\n"
"Hg9j1uliutZfVS7qXMYoCAQlObgOK6nyTJccBz8NUvXt7y+CDwIDAQABo0IwQDAd\n"
"BgNVHQ4EFgQUU3m/WqorSs9UgOHYm8Cd8rIDZsswDgYDVR0PAQH/BAQDAgEGMA8G\n"
"A1UdEwEB/wQFMAMBAf8wDQYJKoZIhvcNAQEMBQADggIBAFzUfA3P9wF9QZllDHPF\n"
"Up/L+M+ZBn8b2kMVn54CVVeWFPFSPCeHlCjtHzoBN6J2/FNQwISbxmtOuowhT6KO\n"
"VWKR82kV2LyI48SqC/3vqOlLVSoGIG1VeCkZ7l8wXEskEVX/JJpuXior7gtNn3/3\n"
"ATiUFJVDBwn7YKnuHKsSjKCaXqeYalltiz8I+8jRRa8YFWSQEg9zKC7F4iRO/Fjs\n"
"8PRF/iKz6y+O0tlFYQXBl2+odnKPi4w2r78NBc5xjeambx9spnFixdjQg3IM8WcR\n"
"iQycE0xyNN+81XHfqnHd4blsjDwSXWXavVcStkNr/+XeTWYRUc+ZruwXtuhxkYze\n"
"Sf7dNXGiFSeUHM9h4ya7b6NnJSFd5t0dCy5oGzuCr+yDZ4XUmFF0sbmZgIn/f3gZ\n"
"XHlKYC6SQK5MNyosycdiyA5d9zZbyuAlJQG03RoHnHcAP9Dc1ew91Pq7P8yF1m9/\n"
"qS3fuQL39ZeatTXaw2ewh0qpKJ4jjv9cJ2vhsE/zB+4ALtRZh8tSQZXq9EfX7mRB\n"
"VXyNWQKV3WKdwrnuWih0hKWbt5DHDAff9Yk2dDLWKMGwsAvgnEzDHNb842m1R0aB\n"
"L6KCq9NjRHDEjf8tM7qtj3u1cIiuPhnPQCjY/MiQu12ZIvVS5ljFH4gxQ+6IHdfG\n"
"jjxDah2nGN59PRbxYvnKkKj9\n"
"-----END CERTIFICATE-----\n"
};

BridgeTCPClient<> client(Bridge);

void setup() {
  /* Initialize Serial */
  Serial.begin(115200);
  /* Initialize RPC transport Serial1 */
  //Serial1.begin(115200);
  if (!Bridge.begin()) {
    Serial.println("cannot setup Bridge");
    while (true) {}
  }
  /* Configure TLS CA */
  //client.setCACert(ubi);
}

void loop() {
  Serial.println("\nStarting connection to server...");
  /* if you get a connection, report back via serial: */
  if (client.connectSSL("ubidefeo.com", 443, ubi) < 0) {
    Serial.println("unable to connect to server");
    return;
  }

  Serial.println("connected to server");
  /* Make aHTTP request: */
  size_t w = client.write("GET /files/supsi.txt HTTP/1.1\n");
  w += client.println("Host: ubidefeo.com");
  w += client.println("User-Agent: Arduino");
  w += client.println("Connection: close");
  w += client.println();

  /* if there are incoming bytes available  from the server,
   * read them and print them:
   */
  while (client.connected()) {
    size_t len = client.available();
    if (len) {
      uint8_t buff[len];
      client.read(buff, len);
      Serial.write(buff, len);
    }
    delay(0);
  }

  /* if the server's disconnected, stop the client: */
  Serial.println();
  Serial.println("disconnecting from server.");
  client.stop();
  delay(1000);
}
