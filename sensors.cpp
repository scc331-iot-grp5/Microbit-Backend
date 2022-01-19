#include "MicroBit.h"

#define DATA                 1
#define DATA_REQUEST         2
#define NEAR_BY_BOAT_REQUEST 3
#define NEAR_BY_BOAT_DATA    4
#define RSSI_DATA            5

#define CRASH_EVENT          10
#define SHAKE_THRESHOLD      150

MicroBit uBit;
ManagedString serial;
ManagedString emptySerialValue("0000");
int messageCount = 0;

int microbitType = 0;//default as sensor

int recordTemp = 0; // 0 = dont record
int recordDirection = 0;
int recordAcc = 0;

//request for temp, directional bearing.
    //on signal - request rssi (sensor to sensor)
    //on return signal - get rssi (sensor to infrastructure)

class MakeCodeMicrophoneTemplate {
  public:
    MIC_DEVICE microphone;
    LevelDetectorSPL level;
    MakeCodeMicrophoneTemplate() MIC_INIT { MIC_ENABLE; }
};
bool inRange(unsigned low, unsigned high, unsigned x)        
{        
 return (low <= x && x <= high);         
} 

ManagedString getDirection()
{
    int x =  uBit.compass.heading();
    if(inRange(349,360,x) || inRange(0,11,x)){
        return "N--";
    }
    else if(inRange(12,33,x)){
        return "NNE";
    }
    else if(inRange(34,56,x)){
        return "NE-";
    }
    else if(inRange(57,78,x)){
        return "ENE";
    }
    else if(inRange(79,101,x)){
        return "E--";
    }
    else if(inRange(102,123,x)){
        return "ESE";
    }
    else if(inRange(124,146,x)){
        return "SE-";
    }
    else if(inRange(147,168,x)){
        return "SSE";
    }
    else if(inRange(169,191,x)){
        return "S--";
    }
    else if(inRange(192,213,x)){
        return "SSW";
    }
    else if(inRange(214,236,x)){
        return "SW-";
    }
    else if(inRange(237,258,x)){
        return "WSW";
    }
    else if(inRange(259,281,x)){
        return "W--";
    }
    else if(inRange(282,303,x)){
        return "WNW";
    }
    else if(inRange(304,326,x)){
        return "NW-";
    }
    else if(inRange(327,348,x)){
        return "NNW";
    }
	return "---";
    //for loop of each section increase 
}
ManagedString convertDirection(char data, char data2, char data3)
{
    ManagedString sh1(data);
    ManagedString sh2(data2);
    ManagedString sh3(data3);

    ManagedString finalDirection(sh1+sh2+sh3);
    return finalDirection;
}	
ManagedString convertSerials(char data, char data2, char data3, char data4)
{
    ManagedString sh1(data); 
    ManagedString sh2(data2);
    ManagedString sh3(data3);
    ManagedString sh4(data4);
    
    ManagedString finalSerial(sh1+sh2+sh3+sh4);
    return finalSerial;
}
bool broadcast(PacketBuffer b)
{
    int resp = uBit.radio.datagram.send(b);
    if (resp == MICROBIT_OK) {
        //uBit.display.print(success);
    } else {
        //uBit.display.print(fail);
    }
    return resp;
}
void requestNearByBoats()
{
    PacketBuffer data(9);
    uint8_t *buf = data.getBytes();
    int msgType = NEAR_BY_BOAT_REQUEST;
    memcpy(buf, &msgType, 1);
    memcpy(buf+1, serial.toCharArray(), 4);
    memcpy(buf+5, emptySerialValue.toCharArray(), 4);

    broadcast(data);
}
int getAccerlation()
{
    int x = uBit.accelerometer.getX();
    int y = uBit.accelerometer.getY();
    int z = uBit.accelerometer.getZ();

    return sqrt((x^2) + (y^2) + (z^2));
}
void sendCrashSignal()
{		
	PacketBuffer data(9);
	uint8_t *buf = data.getBytes();
	int msgType = CRASH_EVENT;
	memcpy(buf, &msgType, 1);
	memcpy(buf+1, serial.toCharArray(), 4);
	memcpy(buf+5, emptySerialValue.toCharArray(), 4);	

	broadcast(data);
}
void checkCollision(MicroBitEvent e)
{	
    int acceleration = getAccerlation();
	if(acceleration>SHAKE_THRESHOLD)
    {
		sendCrashSignal();
	}  
}
void requestData()
{      
    PacketBuffer data(13);
    uint8_t *buf = data.getBytes();
    int msgType = DATA_REQUEST;
    memcpy(buf, &msgType, 1);
	memcpy(buf+1, serial.toCharArray(), 4);
	memcpy(buf+5, emptySerialValue.toCharArray(), 4);

    broadcast(data);
    //uBit.serial.printf("I'm hedddddddddddddre\n");
}
void receive(MicroBitEvent e)
{
    PacketBuffer p = uBit.radio.datagram.recv();
    
    if(microbitType == 0)
    {   
        //uBit.serial.printf("not here\n");
        //------------------
        if(p[0] == DATA_REQUEST && (convertSerials(p[5],p[6],p[7],p[8]) == "0000"))
        {
            ManagedString source(convertSerials(p[1],p[2],p[3],p[4]));
            ManagedString direction;
            ManagedString temp;
            ManagedString acc(getAccerlation());

            if(recordTemp == 1)
            {
                temp = uBit.thermometer.getTemperature();
            }
            if(recordDirection == 1)
            {
                direction = getDirection();
            }
            if(recordAcc == 0)
            {
                acc = "---";
                //uBit.serial.printf("not recording acc");
            }
            if(recordTemp == 0)
            {
                temp = "---";
                //uBit.serial.printf("not recording temp");
            }
            if(recordDirection == 0)
            {
                direction = "---";
                //uBit.serial.printf("not recording direction");
            }

            PacketBuffer data(18);
            uint8_t *buf = data.getBytes();
            int msgType = DATA;
            memcpy(buf, &msgType, 1);
            memcpy(buf+1, serial.toCharArray(), 4);
            memcpy(buf+5, source.toCharArray(), 4);
            memcpy(buf+9, direction.toCharArray(), 3);
            memcpy(buf+12, temp.toCharArray(), 3);
            memcpy(buf+15, acc.toCharArray(), 3);

            broadcast(data);

            requestNearByBoats();
        }
        else if(p[0] == NEAR_BY_BOAT_REQUEST)
        {
            ManagedString source(convertSerials(p[1],p[2],p[3],p[4]));//destination serial
                
            PacketBuffer data(13);
            uint8_t *buf = data.getBytes();
            int msgType = NEAR_BY_BOAT_DATA;
            memcpy(buf, &msgType, 1);
            memcpy(buf+1, serial.toCharArray(), 4);
            memcpy(buf+5, source.toCharArray(), 4);
            //memcpy(buf+9, pairedMbitSerial.toCharArray(),4);//for use on mBuoy
            broadcast(data);
        }
        else if(p[0] == NEAR_BY_BOAT_DATA && (convertSerials(p[5],p[6],p[7],p[8]) == serial))
        {
            ManagedString NBBSerial(convertSerials(p[1],p[2],p[3],p[4]));//serial of nearby boat
            int rssiValue = p.getRSSI();
            ManagedString n(rssiValue);

            PacketBuffer data(16);
            uint8_t *buf = data.getBytes();
            int msgType = RSSI_DATA;
            memcpy(buf, &msgType, 1);
            memcpy(buf+1, serial.toCharArray(), 4);
            memcpy(buf+5, emptySerialValue.toCharArray(), 4);
            memcpy(buf+9, NBBSerial.toCharArray(), 4);
            memcpy(buf+13, n.toCharArray(), 3);
                
            broadcast(data);
        }
    }
    else if (microbitType == 1)
    {
        //uBit.serial.printf("I'm here\n");
        if ((p[0] == DATA) && (convertSerials(p[5],p[6],p[7],p[8]) == serial))
	    {
            ManagedString comma(",");
            ManagedString objSerial(convertSerials(p[1],p[2],p[3],p[4]));
            ManagedString Direction(convertDirection(p[9],p[10],p[11]));
            ManagedString temp(convertDirection(p[12],p[13],p[14]));
            ManagedString acc(convertDirection(p[15],p[16],p[17]));

            ManagedString header("Data");
            ManagedString end("\n");
            ManagedString ID(messageCount);
            int rssiValue = p.getRSSI();
            ManagedString n(rssiValue);
            
            ManagedString message = ID + comma +header + comma + serial + comma + objSerial + comma + Direction + comma + temp + comma + acc + comma + n + end; // message to send to webserver
            messageCount = messageCount +1;

            uBit.serial.printf(message.toCharArray());
        }
        else if ((p[0] == RSSI_DATA) && (convertSerials(p[5],p[6],p[7],p[8]) == "0000"))
	    {
            ManagedString comma(",");
            ManagedString objSerial(convertSerials(p[1],p[2],p[3],p[4]));
            ManagedString nearBoatSerial(convertSerials(p[9],p[10],p[11],p[12]));
            ManagedString rssiValue(convertDirection(p[13],p[14],p[15]));
            ManagedString header("NBBData");
            ManagedString end("\n");
            ManagedString ID(messageCount);

            ManagedString message = ID + comma + header + comma + objSerial + comma + nearBoatSerial + comma + rssiValue + comma +end;
            messageCount = messageCount +1;
            uBit.serial.printf(message.toCharArray());
        }
	else if (p[0] == CRASH_EVENT && (convertSerials(p[5],p[6],p[7],p[8]) == emptySerialValue))
	{
		ManagedString comma(",");
		ManagedString header("Crash");
		ManagedString boatSerial(convertSerials(p[1],p[2],p[3],p[4]));
		ManagedString ID(messageCount);
	        ManagedString end("\n");

		ManagedString message = ID + comma + header + comma + boatSerial + comma +end; // message to send to webserver

		messageCount = messageCount +1;

		uBit.serial.printf(message.toCharArray());

    	}
    }
}
void recieveLoop()
{
    ManagedString end("\n");
    ManagedString x;
    //uBit.serial.printf("I'm here\n");
    while(1)
    {
        //uBit.serial.printf("I'm in here\n");
        x = uBit.serial.readUntil(end);
        //uBit.serial.printf(x.toCharArray());
        if(x == 1){//record temp
            recordTemp = 1;
        }
        if(x == 2){//dont record temp
            recordTemp = 0;
        }
        if(x == 3){//record direc
            recordDirection = 1;
        }
        if(x == 4){//dont record direc
            recordDirection = 0;
        }
        if(x == 5){//record aCC
            recordAcc = 1;
        }
        if(x == 6){//dont record ACC
            recordAcc = 0;
        }
        if(x == 7){//record TYPE
            microbitType = 1;
        }
        if(x == 8){//dont record TYPE
            microbitType = 0;
        }
        //uBit.sleep(2500);

        
        /*int len =  x.length();
        address = x.substring(0,4);
        uBit.serial.printf(address.toCharArray());
        messageToSend = x.substring(4,len-4);
        uBit.serial.printf(messageToSend.toCharArray());
        create_fiber(sendMessage);
        address = "1111";*/
    }
}
int main()
{
    uBit.init();
    uBit.serial.setBaud(115200);
    uBit.compass.calibrate();
   
    ManagedString s(uBit.getSerial());
    serial = s.substring(9,4);
	
	uBit.messageBus.listen(MICROBIT_ID_RADIO, MICROBIT_RADIO_EVT_DATAGRAM, receive);
	//uBit.messageBus.listen(DEVICE_ID_BUTTON_A, DEVICE_BUTTON_EVT_CLICK, onButtonA);//set group
	//uBit.messageBus.listen(DEVICE_ID_BUTTON_B, DEVICE_BUTTON_EVT_CLICK, onButtonB);//set pin
    //uBit.messageBus.listen(DEVICE_ID_BUTTON_B, MICROBIT_BUTTON_EVT_HOLD, onButtonBHold);//confirm number
 	//uBit.messageBus.listen(DEVICE_ID_BUTTON_A, MICROBIT_BUTTON_EVT_LONG_CLICK, onButtonAHold);//display boat serial

	new MakeCodeMicrophoneTemplate(); 
    uBit.messageBus.listen(DEVICE_ID_MICROPHONE, LEVEL_THRESHOLD_HIGH,checkCollision);

    uBit.radio.enable();
	
	uBit.sleep(1000);
    //uBit.serial.printf(" here\n");

    create_fiber(recieveLoop);
    while(1)
    {	
		uBit.sleep(10000);	
        if (microbitType == 1)
        {
            requestData();
        }
        //uBit.serial.printf("I'm \n");
    }
}
	
