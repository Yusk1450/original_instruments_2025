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
 myRemoteLocation = new NetAddress("192.168.0.103", 4444); 
  //myRemoteLocation = new NetAddress("192.168.0.104", 4000); 
 oscP5 = new OscP5(this,4444); //受信するポートの設定
  //oscP5 = new OscP5(this,4000); //受信するポートの設定
 
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
