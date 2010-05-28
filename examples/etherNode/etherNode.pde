// Arduino demo sketch for testing RFM12B + ENC28J60 ethernet
// Listens for RF12 messages and displays valid messages on a webpage
// Memory usage exceeds 1K, so use Atmega328 or decrease history/buffers
//
// This sketch is derived from RF12eth.pde:
// May 2010, Andras Tucsni, http://opensource.org/licenses/mit-license.php
//
// The EtherCard library is based on Guido Socher's driver, licensed as GPL2.
//
// Mods bij jcw, 2010-05-20
 
#include <EtherCard.h>
#include <Ports.h>
#include <RF12.h>

// ethernet interface mac address
static byte mymac[6] = { 0x54,0x55,0x58,0x10,0x00,0x26 };

// ethernet interface ip address
static byte myip[4] = { 192,168,178,203 };

// listen port for tcp/www:
#define HTTP_PORT 80

static word refresh = 5; // homepage refresh rate

// RF12 settings
static byte freq = RF12_868MHZ;
static byte group = 5;
#define MYNODE 31

#define NUM_MESSAGES  10    // Number of messages saved in history
#define MESSAGE_TRUNC 15    // Truncate message payload here to spare memory

static byte buf[1000];
static BufferFiller bfill;

static byte history_rcvd[NUM_MESSAGES][MESSAGE_TRUNC+1]; //history record
static byte history_len[NUM_MESSAGES]; // # of RF12 messages+header in history
static byte next_msg;            //Pointer to next rf12rcvd line
static word msgs_rcvd;       //Total number of received lines

EtherCard eth;

void setup(){
    // Serial.begin(57600);
    // Serial.println("\n[etherNode]");
    rf12_initialize(MYNODE, freq, group);
    /* init ENC28J60, must be done after SPI has been properly set up! */
    eth.initialize(mymac);
    eth.initIp(mymac, myip, HTTP_PORT);
}

char okHeader[] PROGMEM = 
    "HTTP/1.0 200 OK\r\n"
    "Content-Type: text/html\r\n"
    "Pragma: no-cache\r\n"
;

static void homePage(BufferFiller& buf) {
    word mhz = freq == 1 ? 433 : freq == 2 ? 868 : 915;
    buf.emit_p(PSTR("$F\r\n"
        "<meta http-equiv='refresh' content='$D'/>"
        "<title>RF12 etherNode - $D MHz, group $D</title>" 
        "RF12 etherNode - $D MHz, group $D - (<a href='c'>configure</a>)"
        "<h3>Last $D messages:</h3>"
        "<pre>"), okHeader, refresh, mhz, group, mhz, group, NUM_MESSAGES);
    for (byte i = 0; i < NUM_MESSAGES; ++i) {
        byte j = (next_msg + i) % NUM_MESSAGES;
        if (history_len[j] > 0) {
            word num = msgs_rcvd - NUM_MESSAGES + i + 1000;
            buf.emit_p(PSTR("\n$D: OK"), num);
            for (byte k = 0; k < history_len[j]; ++k)
                buf.emit_p(PSTR(" $D"), history_rcvd[j][k]);
        }
    }
    long t = millis() / 1000;
    word h = t / 3600;
    byte m = (t / 60) % 60;
    byte s = t % 60;
    buf.emit_p(PSTR(
        "</pre>"
        "Uptime is $D$D:$D$D:$D$D"), h/10, h%10, m/10, m%10, s/10, s%10);
}

static int getIntArg(const char* data, const char* key, int value =-1) {
    char temp[10];
    if (find_key_val(data + 7, temp, sizeof temp, key) > 0)
        value = atoi(temp);
    return value;
}

static void configPage(const char* data, BufferFiller& buf) {
    // pick up submitted data, if present
    if (data[6] == '?') {
        byte b = getIntArg(data, "b");
        byte g = getIntArg(data, "g");
        word r = getIntArg(data, "r");
        if (1 <= g && g <= 250 && 1 <= r && r <= 3600) {
            // use values as new settings
            freq = b == 8 ? RF12_868MHZ : b == 9 ? RF12_915MHZ : RF12_433MHZ;
            group = g;
            refresh = r;
            rf12_initialize(MYNODE, freq, group);
            // clear history
            memset(history_len, 0, sizeof history_len);
            // redirect to the home page
            buf.emit_p(PSTR(
                "HTTP/1.0 302 found\r\n"
                "Location: /\r\n"
                "\r\n"));
            return;
        }
    }
    // else show a configuration form
    byte band = freq == 1 ? 4 : freq == 2 ? 8 : 9;
    buf.emit_p(PSTR("$F\r\n"
        "<h3>Server node configuration</h3>"
        "<form>"
          "<p>"
    "Freq band <input type=text name=b value='$D' size=1> (4, 8, or 9)<br>"
    "Net group <input type=text name=g value='$D' size=3> (1..250)<br>"
    "Refresh rate <input type=text name=r value='$D' size=4> (1..3600 seconds)"
          "</p>"
          "<input type=submit value=Set>"
        "</form>"), okHeader, band, group, refresh);
}

void loop(){
    word len = eth.packetReceive(buf, sizeof buf);
    // ENC28J60 loop runner: handle ping and wait for a tcp packet
    word pos = eth.packetLoop(buf,len);
    // check if valid tcp data is received
    if (pos) {
        bfill = eth.tcpOffset(buf);
        char* data = (char *) buf + pos;
        // receive buf hasn't been clobbered by reply yet
        if (strncmp("GET / ", data, 6) == 0)
            homePage(bfill);
        else if (strncmp("GET /c", data, 6) == 0)
            configPage(data, bfill);
        else
            bfill.emit_p(PSTR(
                "HTTP/1.0 401 Unauthorized\r\n"
                "Content-Type: text/html\r\n"
                "\r\n"
                "<h1>401 Unauthorized</h1>"));  
        eth.httpServerReply(buf,bfill.position()); // send web page data
    }

    // RFM12 loop runner, don't report acks
    if (rf12_recvDone() && rf12_crc == 0 && (rf12_hdr & RF12_HDR_CTL) == 0) {
        history_rcvd[next_msg][0] = rf12_hdr;
        for (byte i = 0; i < rf12_len; ++i)
            if (i < MESSAGE_TRUNC) 
                history_rcvd[next_msg][i+1] = rf12_data[i];
        history_len[next_msg] = rf12_len < MESSAGE_TRUNC ? rf12_len+1
                                                         : MESSAGE_TRUNC+1;
        next_msg = (next_msg + 1) % NUM_MESSAGES;
        msgs_rcvd = (msgs_rcvd + 1) % 9000;
    }
}
