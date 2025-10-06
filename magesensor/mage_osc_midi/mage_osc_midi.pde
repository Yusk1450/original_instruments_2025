//************************************
//osc(receive)→midi
//************************************

import oscP5.*;
import netP5.*;
import themidibus.*;
  
OscP5 oscP5;
NetAddress myRemoteLocation;

int val = 0;

MidiBus myBus;//MidiBusインスタンスを作成
int midiChannel = 1;//使用するMIDIチャンネル
int note = -1;

int playMode = 1;
 
void setup() {
 size(400, 400);
 frameRate(25);
 myRemoteLocation = new NetAddress("192.168.0.101", 8000); 
 oscP5 = new OscP5(this,8000); //受信するポートの設定
 
 //myRemoteLocation = new NetAddress("192.168.0.102", 8888); 
 //oscP5 = new OscP5(this,8000); //受信するポートの設定
 
 MidiBus.list();
 myBus = new MidiBus();
 myBus.registerParent(this);
 myBus.addInput(1);
 myBus.addOutput(2);
  
 println("設定が完了しました。");
 
}
 
void draw() {
 background(200);

}
 
//OSC
void oscEvent(OscMessage theOscMessage) {
  // アドレスパターンをチェック
  if (theOscMessage.checkAddrPattern("/note") == true) {
    note = theOscMessage.get(0).intValue();
    println("onNOte:", note);
     myBus.sendNoteOn(midiChannel, note, 127);
  }
  else if (theOscMessage.checkAddrPattern("/off") == true)
  {
    note = theOscMessage.get(0).intValue();
    myBus.sendNoteOn(midiChannel, note, 0);
    println("offNote:", note);
  }
  else {
    println("指定してないアドレス：" + theOscMessage.addrPattern());
  }
}

void keyPressed() {
  if (key == '1') playMode = 1;
  if (key == '2') playMode = 2;
  if (key == '3') playMode = 3;
  if (key == '4') playMode = 4;
  
  OscMessage msg = new OscMessage("/playMode");
  msg.add(playMode); 
  oscP5.send(msg, myRemoteLocation);
  println("Send /playMode " + playMode);
}
