#include <EtherCard.h>

#include <string.h>

#define KEY "s3cr1t"

#define BUTTON_PIN (3)
#define BUTTON_PRESS (HIGH)

/* digitalWrite(LED_PIN, LOW); --> turns on the LED */
//#define LED_PIN    (6) /* status led for nanode */
#define LED_PIN    (5)

#define BLINK_CLOCK (250) /* milliseconds */

#define DEBOUNCE_WAIT (200)

#define DNS_TIMEOUT (15000)
#define DNS_TTL (3600000)
#define CONNECT_TIMEOUT (15000)
#define RECONNECT_INTERVAL (30000)

static const uint8_t mac[] = "TEXINC" ; /* first byte should be even */

uint8_t Ethernet::buffer[786];

static const prog_char server_name[] PROGMEM = "www.techinc.nl";
static const prog_char open_path[]   PROGMEM = "/space/?state=open&key=" KEY;
static const prog_char close_path[]  PROGMEM = "/space/?state=closed&key=" KEY;
static const prog_char check_path[]  PROGMEM = "/space/spacestate";

enum
{
	SPACE_UNKNOWN,
	SPACE_OPENING,
	SPACE_OPEN,
	SPACE_CLOSING,
	SPACE_CLOSED,

} space_state = SPACE_UNKNOWN;

enum
{
	NETWORK_NO_LINK,
	NETWORK_LINK,
	NETWORK_DHCP,
	NETWORK_RECONNECT,
	NETWORK_CONNECTING,
	NETWORK_OK,

} network_state = NETWORK_NO_LINK;

enum
{
	BLINK_NO_LINK  = 0x0001,
	BLINK_LINK     = 0x0101,
	BLINK_DHCP     = 0x1111,
	BLINK_UNKNOWN  = 0xff00,
	BLINK_OPEN     = 0xffff,
	BLINK_CLOSED   = 0x0000,
	BLINK_CHANGING = 0x5555,

} blink_mode = BLINK_NO_LINK;

uint8_t button_state = (HIGH+LOW) - BUTTON_PRESS;
long button_debounce;

long cur; /* current loop's timestamp */

long last_dns, last_update;

/* search for open / closed in document body */
static void request_callback(byte status, word off, word len)
{
	if (status != 0) /* error */
		return;

	Ethernet::buffer[sizeof(Ethernet::buffer)-1] = 0;

	/* Scan for HTTP body */
	while ( (len >= 4) && (Ethernet::buffer[off] != '\0') )
	{
		if ( (Ethernet::buffer[off+0] == '\x0d') &&
		     (Ethernet::buffer[off+1] == '\x0a') &&
		     (Ethernet::buffer[off+2] == '\x0d') &&
		     (Ethernet::buffer[off+3] == '\x0a') )
			break;
		off++;
		len--;
	}

	if (strstr((const char*)Ethernet::buffer+off, "open"))
		space_state = SPACE_OPEN;
	else if (strstr((const char*)Ethernet::buffer+off, "closed"))
		space_state = SPACE_CLOSED;
	else
		space_state = SPACE_UNKNOWN;

	network_state = NETWORK_OK;
}

void do_request()
{
	const prog_char PROGMEM *path = check_path;

	if (space_state == SPACE_CLOSING)
		path = close_path;

	if (space_state == SPACE_OPENING)
		path = open_path;

	ether.browseUrl((char PROGMEM *)path, "", (char PROGMEM *)server_name, request_callback);
}

void network()
{
	if (!Ethernet::isLinkUp())
	{
		network_state = NETWORK_NO_LINK;
		return;
	}

	if ( ( (network_state >= NETWORK_DHCP) && !EtherCard::dhcpPoll() ) ||
	     ( (network_state == NETWORK_DHCP) && (cur-last_dns > DNS_TIMEOUT) ) ||
	     ( (network_state == NETWORK_CONNECTING) && (cur-last_update > CONNECT_TIMEOUT) ) )
		network_state = NETWORK_NO_LINK;

	if  ( (network_state > NETWORK_DHCP) && (cur-last_dns > DNS_TTL) )
		network_state = NETWORK_LINK;

	if  ( (network_state == NETWORK_OK) && (cur-last_update > RECONNECT_INTERVAL) )
		network_state = NETWORK_RECONNECT;

	switch (network_state)
	{
		case NETWORK_NO_LINK:
			network_state = NETWORK_LINK;
			EtherCard::dhcpAsync("spacebutton");
			/* fall through */

		case NETWORK_LINK:
			if (!EtherCard::dhcpPoll())
				break;
			network_state = NETWORK_DHCP;
			EtherCard::dnsLookupAsync(server_name);
			last_dns = cur;
			/* fall through */

		case NETWORK_DHCP:
			if (!EtherCard::dnsLookupPoll())
				break;

			/* fall through */

		case NETWORK_RECONNECT:
			network_state = NETWORK_CONNECTING;
			do_request();
			last_update = cur;
			/* fall through */

		default:
			ether.packetLoop(ether.packetReceive());
	}
}

void button_press()
{
	switch (space_state)
	{
		case SPACE_UNKNOWN:
		case SPACE_CLOSING:
		case SPACE_CLOSED:
			space_state = SPACE_OPENING;
			break;

		case SPACE_OPENING:
		case SPACE_OPEN:
			space_state = SPACE_CLOSING;
			break;
	}

	if ( network_state >= NETWORK_CONNECTING )
		network_state = NETWORK_RECONNECT;
}

void button()
{
	if ( (digitalRead(BUTTON_PIN) != button_state) &&
	     (cur-button_debounce > DEBOUNCE_WAIT) )
	{
		button_state = HIGH+LOW - button_state;
		button_debounce = cur;
		if (button_state == BUTTON_PRESS)
			button_press();
	}
}

void blink()
{
	static uint8_t bit=0;
	static long last=0;

	if (cur-last < BLINK_CLOCK)
		return;

	last = cur;

	switch (space_state)
	{
		case SPACE_UNKNOWN:
			blink_mode = BLINK_UNKNOWN;
			break;
		case SPACE_OPENING:
		case SPACE_CLOSING:
			blink_mode = BLINK_CHANGING;
			break;
		case SPACE_OPEN:
			blink_mode = BLINK_OPEN;
			break;
		case SPACE_CLOSED:
			blink_mode = BLINK_CLOSED;
		default:
			break;
	}

	switch (network_state)
	{
		case NETWORK_NO_LINK:
			blink_mode = BLINK_NO_LINK;
			break;
		case NETWORK_LINK:
			blink_mode = BLINK_LINK;
			break;
		case NETWORK_DHCP:
			blink_mode = BLINK_DHCP;
			break;
		default:
			break;
	}

	digitalWrite(LED_PIN, (blink_mode&(1<<bit)) ? LOW : HIGH);

	bit++;
	bit &= 0xf;
}

void setup()
{
	pinMode(BUTTON_PIN, INPUT);
	digitalWrite(BUTTON_PIN, HIGH); /* pull-up resistor */
	button_debounce = millis()-DEBOUNCE_WAIT-1;

	pinMode(LED_PIN, OUTPUT);
	digitalWrite(LED_PIN, HIGH);

	ether.begin(sizeof(Ethernet::buffer), mac);
}

void loop()
{
	cur = millis();
	blink();
	button();
	network();
}

