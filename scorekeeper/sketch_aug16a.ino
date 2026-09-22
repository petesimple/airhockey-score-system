/* TD Air Hockey Table Scorecard
   Target: ESP32-2432S028-style CYD, 240x320 resistive touch
   Board: ESP32 Dev Module
   Libraries: TFT_eSPI (Bodmer), XPT2046_Touchscreen (Paul Stoffregen)
*/

#include <SPI.h>
#include <WiFi.h>
#include <WiFiUdp.h>
#include <HTTPClient.h>
#include <ArduinoJson.h>
#include <Preferences.h>
#include <time.h>
#include <TFT_eSPI.h>
#include <XPT2046_Touchscreen.h>
#include "scorekeeper_logo.h"

TFT_eSPI tft;
constexpr int TFT_BL = 21;
constexpr int T_CLK=25, T_MISO=39, T_MOSI=32, T_CS=33, T_IRQ=36;
SPIClass touchSPI(VSPI);
XPT2046_Touchscreen touch(T_CS, T_IRQ);

// Air Hockey Scorekeeper 7-6 brand palette.
constexpr uint16_t BG=0x00C5, PANEL=0x0947, PANEL2=0x11A9, LINE=0x326B;
constexpr uint16_t TEXT=0xFFBC, MUTED=0xAD96, LIME=0xFFBC, ORANGE=0xD127, RED=0xD127, BLUE=0x0BD7;
constexpr int TX_MIN=250, TX_MAX=3850, TY_MIN=250, TY_MAX=3850;

enum Screen { SETUP, MANUAL_TABLES, MANUAL_MATCH, MANUAL_NAMES, TABLE_PIN, TABLE_SELECT, HISTORY_LIST, HISTORY_DETAIL, INITIALS, WIFI_LIST, WIFI_PASSWORD, HUB_SETUP, GAMES_MENU, PHOTON_NAMES, PHOTON_GAME, PHOTON_RESULT, SCORE, CONFIRM_GAME, SWITCH_SIDES, TIMEOUT_SCREEN, RESULT };
Screen screen=SETUP;
const char* DATA_URL="https://script.google.com/macros/s/AKfycbyjcgJgD_OSobFfk3Vk4Tp5Pt9zgYJbsvh_vn9PvVDtni-3KKgwKqrMFk7PAtiXkxke/exec?action=publicBracketGet&tournamentId=7-6-weekly";

constexpr int SCOREKEEPER_HUB_PORT=8081;
constexpr int SCOREKEEPER_DISCOVERY_PORT=8082;

Preferences preferences;
constexpr int MAX_SAVED_NETWORKS=5;
String savedSSID[MAX_SAVED_NETWORKS],savedPassword[MAX_SAVED_NETWORKS];
int savedNetworkCount=0;
String pendingSSID="",pendingPassword="";
String hubIP="",hubTestMessage="",wifiMessage="";
bool hubConnected=false;
int lastHubAssignmentRevision=0;
unsigned long lastHubAssignmentPoll=0;
constexpr int MAX_WIFI_NETWORKS=20;
constexpr int WIFI_NETWORKS_PER_PAGE=4;
String networkName[MAX_WIFI_NETWORKS];
bool networkOpen[MAX_WIFI_NETWORKS]={false};
bool networkSaved[MAX_WIFI_NETWORKS]={false};
int networkRSSI[MAX_WIFI_NETWORKS]={0};
int networkCount=0,wifiPage=0,keyboardMode=0;
bool autoWifiEnabled=false,autoWifiTrying=false,autoWifiExhausted=false;
int autoWifiIndex=0,autoWifiTry=0;
unsigned long autoWifiAttemptStarted=0;

struct CalledMatch { String table,p1,p2,status,matchId,matchType; bool fromHub=false; };
constexpr int MAX_CALLED_MATCHES=30;
constexpr int MATCHES_PER_PAGE=4;
CalledMatch matches[MAX_CALLED_MATCHES];
int matchCount=0,matchPage=0,selectedTable=0,bestOf=3;
bool isManualMatch=false;
int venueTableCount=5,deviceTable=1;
String tablePin="",tablePinMessage="";
bool tableChangeFromManual=false;
int historyCount=0,selectedHistory=0,historyFilter=0,historyPage=0,historyPageCount=1,historyVisibleCount=0;
int historyVisibleIndex[4]={0,0,0,0};
bool historySaved=false,photonHistorySaved=false;
bool lastStandardMatchAvailable=false,editingCompletedMatch=false;
bool manualNameEdited[2]={false,false};
const char* TZ_INFO="CST6CDT,M3.2.0/2,M11.1.0/2";
String player[2]={"",""};
int editingPlayer=0,manualKeyboardMode=0;

// Scores are stored by player identity; leftPlayer says who is physically left.
int leftPlayer=0, sideScore[2]={0,0}, gamesWon[2]={0,0};
int gameScore[2][9]={{0},{0}};
int completedGames=0;
unsigned long timeoutEnds=0;
int lastTimeoutSecond=-1;
bool playerTimeoutUsed[2]={false,false};
String timeoutLabel="TIME OUT";

int photonScore[2]={0,0},photonGoalCount[2]={0,0};
int photonGamesWon[2]={0,0},photonEditingTeam=0,photonKeyboardMode=0;
int photonGameScore[2][3]={{0},{0}},photonCompletedGames=0,photonTimeoutTeam=-1;
String photonName[2]={"RED TEAM","BLUE TEAM"};
bool photonNameEdited[2]={false,false},photonMatchComplete=false,photonTimeoutUsed[2]={false,false},photonSidesSwapped=false;
uint8_t photonGoalValue[2][30]={{0},{0}};
unsigned long photonRemainingMs=180000,photonLastTick=0,photonTimeoutEnds=0;
bool photonRunning=false,photonBlitzReported=false,photonSuddenDeath=false,photonTimeoutActive=false;

struct Button { int x,y,w,h; const char* label; uint16_t fill,ink; };
bool hit(const Button& b,uint16_t x,uint16_t y){return x>=b.x&&x<b.x+b.w&&y>=b.y&&y<b.y+b.h;}

void drawButton(const Button& b,int font=2){
  tft.fillRoundRect(b.x,b.y,b.w,b.h,6,b.fill);
  tft.drawRoundRect(b.x,b.y,b.w,b.h,6,b.fill==PANEL?LINE:b.fill);
  tft.setTextDatum(MC_DATUM); tft.setTextColor(b.ink,b.fill);
  tft.drawString(b.label,b.x+b.w/2,b.y+b.h/2,font);
}

void drawStringButton(int x,int y,int w,int h,const String& label,uint16_t fill,uint16_t ink,int font=2){
  tft.fillRoundRect(x,y,w,h,6,fill);tft.drawRoundRect(x,y,w,h,6,fill==PANEL?LINE:fill);
  tft.setTextDatum(MC_DATUM);tft.setTextColor(ink,fill);tft.drawString(label,x+w/2,y+h/2,font);
}

void drawSwapIconButton(){
  int x=145,y=5;tft.fillRoundRect(x,y,30,26,6,PANEL2);tft.drawRoundRect(x,y,30,26,6,LINE);
  tft.drawFastHLine(x+7,y+9,14,TEXT);tft.fillTriangle(x+22,y+6,x+22,y+12,x+26,y+9,TEXT);
  tft.drawFastHLine(x+9,y+17,14,TEXT);tft.fillTriangle(x+8,y+14,x+8,y+20,x+4,y+17,TEXT);
}

void drawRefTimeoutIcon(){
  int x=272,y=39;tft.fillRoundRect(x,y,44,34,6,PANEL2);tft.drawRoundRect(x,y,44,34,6,LINE);
  int cx=x+22,cy=y+17;tft.drawCircle(cx,cy,10,TEXT);tft.drawCircle(cx,cy,9,TEXT);
  tft.drawFastVLine(cx,cy-6,7,TEXT);tft.drawLine(cx,cy,cx+5,cy+3,TEXT);tft.fillCircle(cx,cy,2,TEXT);
}

void header(const char* status){
  tft.fillRect(0,0,320,36,PANEL); tft.drawFastHLine(0,35,320,LINE);
  tft.pushImage(2,3,SCOREKEEPER_LOGO_SMALL_SIZE,SCOREKEEPER_LOGO_SMALL_SIZE,SCOREKEEPER_LOGO_SMALL);
  tft.setTextDatum(TL_DATUM);tft.setTextColor(TEXT,PANEL);tft.drawString("SCOREKEEPER",38,8,2);
  tft.setTextDatum(TR_DATUM); tft.setTextColor(MUTED,PANEL); tft.drawString(status,308,11,2);
}

void drawWifiBars(){
  tft.fillRect(224,4,48,28,PANEL);
  if(autoWifiTrying){
    tft.setTextDatum(MC_DATUM);tft.setTextColor(ORANGE,PANEL);
    tft.drawString("TRY "+String(autoWifiTry)+"/3",248,18,1);
    return;
  }
  int bars=0;
  if(WiFi.status()==WL_CONNECTED){
    int rssi=WiFi.RSSI();bars=rssi>=-55?4:rssi>=-67?3:rssi>=-78?2:1;
  }
  for(int i=0;i<4;i++){
    int h=4+i*3,x=232+i*6,y=27-h;
    tft.fillRect(x,y,4,h,i<bars?LIME:LINE);
  }
  tft.fillCircle(262,18,4,hubConnected?0x07E0:RED);
}

String fitName(String value,int maxLen=15){value.trim();if(value.length()>maxLen)value=value.substring(0,maxLen-1)+".";return value;}

int savedWifiIndex(const String& ssid){
  for(int i=0;i<savedNetworkCount;i++)if(ssid==savedSSID[i])return i;
  return -1;
}

void loadSavedNetworks(){
  // Schema 2 deliberately starts clean, removing the old hard-coded/single profile.
  if(preferences.getUChar("wifiSchema",0)<2){
    preferences.remove("ssid");preferences.remove("password");
    preferences.putUChar("savedCount",0);preferences.putUChar("wifiSchema",2);
  }
  savedNetworkCount=constrain((int)preferences.getUChar("savedCount",0),0,MAX_SAVED_NETWORKS);
  for(int i=0;i<savedNetworkCount;i++){
    savedSSID[i]=preferences.getString(("ssid"+String(i)).c_str(),"");
    savedPassword[i]=preferences.getString(("pass"+String(i)).c_str(),"");
  }
}

void persistSavedNetworks(){
  preferences.putUChar("savedCount",savedNetworkCount);
  for(int i=0;i<MAX_SAVED_NETWORKS;i++){
    String ssidKey="ssid"+String(i),passKey="pass"+String(i);
    if(i<savedNetworkCount){preferences.putString(ssidKey.c_str(),savedSSID[i]);preferences.putString(passKey.c_str(),savedPassword[i]);}
    else{preferences.remove(ssidKey.c_str());preferences.remove(passKey.c_str());}
  }
}

void rememberNetwork(const String& ssid,const String& password){
  int old=savedWifiIndex(ssid);
  if(old<0&&savedNetworkCount<MAX_SAVED_NETWORKS)savedNetworkCount++;
  if(old<0)old=savedNetworkCount-1;
  for(int i=old;i>0;i--){savedSSID[i]=savedSSID[i-1];savedPassword[i]=savedPassword[i-1];}
  savedSSID[0]=ssid;savedPassword[0]=password;persistSavedNetworks();
}

void forgetNetwork(int index){
  if(index<0||index>=savedNetworkCount)return;
  String forgottenSSID=savedSSID[index];
  for(int i=index;i<savedNetworkCount-1;i++){savedSSID[i]=savedSSID[i+1];savedPassword[i]=savedPassword[i+1];}
  savedNetworkCount--;persistSavedNetworks();
  if(WiFi.status()==WL_CONNECTED&&WiFi.SSID()==forgottenSSID)WiFi.disconnect(false,false);
}

void refreshAutoWifiIndicator(){
  if(screen==SETUP||screen==MANUAL_MATCH||screen==GAMES_MENU||screen==WIFI_LIST)drawWifiBars();
}

void beginAutoWifiAttempt(){
  if(!autoWifiEnabled||savedNetworkCount==0||autoWifiIndex>=savedNetworkCount)return;
  autoWifiTrying=true;autoWifiTry++;
  WiFi.disconnect(false,false);WiFi.mode(WIFI_STA);
  WiFi.begin(savedSSID[autoWifiIndex].c_str(),savedPassword[autoWifiIndex].c_str());
  autoWifiAttemptStarted=millis();
  wifiMessage="TRY "+String(autoWifiTry)+"/3: "+fitName(savedSSID[autoWifiIndex],16);
  refreshAutoWifiIndicator();
}

void startAutoWifi(){
  autoWifiTrying=false;autoWifiExhausted=false;autoWifiIndex=0;autoWifiTry=0;
  if(autoWifiEnabled&&savedNetworkCount>0&&WiFi.status()!=WL_CONNECTED)beginAutoWifiAttempt();
  else refreshAutoWifiIndicator();
}

void stopAutoWifi(){
  if(autoWifiTrying)WiFi.disconnect(false,false);
  autoWifiTrying=false;autoWifiExhausted=false;wifiMessage="AUTO WIFI OFF";refreshAutoWifiIndicator();
}

void serviceAutoWifi(){
  if(!autoWifiEnabled)return;
  if(!autoWifiTrying){
    if(WiFi.status()!=WL_CONNECTED&&!autoWifiExhausted&&savedNetworkCount>0)startAutoWifi();
    return;
  }
  if(WiFi.status()==WL_CONNECTED){
    autoWifiTrying=false;wifiMessage="CONNECTED: "+fitName(WiFi.SSID(),18);
    configTzTime(TZ_INFO,"pool.ntp.org","time.google.com");refreshAutoWifiIndicator();return;
  }
  if(millis()-autoWifiAttemptStarted<8000)return;
  if(autoWifiTry<3){beginAutoWifiAttempt();return;}
  autoWifiIndex++;autoWifiTry=0;
  if(autoWifiIndex<savedNetworkCount){beginAutoWifiAttempt();return;}
  autoWifiTrying=false;autoWifiExhausted=true;wifiMessage="AUTO WIFI: 3 TRIES USED";refreshAutoWifiIndicator();
}

bool connectWifiProfile(const String& ssid,const String& password,int attempts=24){
  if(!ssid.length())return false;
  WiFi.mode(WIFI_STA);WiFi.begin(ssid.c_str(),password.c_str());
  for(int i=0;i<attempts&&WiFi.status()!=WL_CONNECTED;i++)delay(500);
  return WiFi.status()==WL_CONNECTED;
}

bool discoverHub(){
  if(WiFi.status()!=WL_CONNECTED)return false;
  WiFiUDP udp;
  if(!udp.begin(0))return false;
  IPAddress local=WiFi.localIP(),mask=WiFi.subnetMask(),broadcastIP;
  for(int i=0;i<4;i++)broadcastIP[i]=local[i]|(~mask[i]);
  const char* request="SCOREKEEPER_HUB_DISCOVER";
  udp.beginPacket(broadcastIP,SCOREKEEPER_DISCOVERY_PORT);
  udp.write((const uint8_t*)request,strlen(request));
  udp.endPacket();
  unsigned long started=millis();
  while(millis()-started<1800){
    int packetSize=udp.parsePacket();
    if(packetSize>0){
      char response[192];int count=udp.read(response,sizeof(response)-1);response[max(0,count)]=0;
      if(String(response).indexOf("scorekeeper-hub")>=0){
        hubIP=udp.remoteIP().toString();preferences.putString("hubIP",hubIP);
        hubConnected=true;udp.stop();return true;
      }
    }
    delay(20);
  }
  udp.stop();return false;
}

bool fetchHubAssignment(bool onlyIfNew=false){
  if(WiFi.status()!=WL_CONNECTED||!hubIP.length())return false;
  String url="http://"+hubIP+":"+String(SCOREKEEPER_HUB_PORT)+"/api/matches/table/"+String(deviceTable)+"?deviceId="+WiFi.macAddress();
  HTTPClient http;http.begin(url);http.setTimeout(1800);int code=http.GET();
  if(code!=HTTP_CODE_OK){http.end();hubConnected=false;return false;}
  String payload=http.getString();http.end();
  JsonDocument doc;if(deserializeJson(doc,payload)||!(doc["ok"]|false))return false;
  if(doc["match"].isNull()){
    bool removed=false;int writeIndex=0;
    for(int i=0;i<matchCount;i++){
      if(matches[i].fromHub){removed=true;continue;}
      if(writeIndex!=i)matches[writeIndex]=matches[i];
      writeIndex++;
    }
    matchCount=writeIndex;lastHubAssignmentRevision=0;
    return removed;
  }
  JsonObject item=doc["match"].as<JsonObject>();int revision=item["revision"]|0;
  hubConnected=true;if(onlyIfNew&&revision==lastHubAssignmentRevision)return false;
  lastHubAssignmentRevision=revision;
  String table=String(item["table"]|deviceTable),p1=String(item["player1"]|"TBD"),p2=String(item["player2"]|"TBD"),matchId=String(item["matchId"]|"");
  int writeIndex=0,duplicateIndex=-1;
  for(int i=0;i<matchCount;i++){
    if(matches[i].fromHub)continue;
    if(writeIndex!=i)matches[writeIndex]=matches[i];
    if((matchId.length()&&matches[writeIndex].matchId==matchId)||(matches[writeIndex].table==table&&matches[writeIndex].p1==p1&&matches[writeIndex].p2==p2))duplicateIndex=writeIndex;
    writeIndex++;
  }
  matchCount=writeIndex;
  if(duplicateIndex>=0){matchPage=duplicateIndex/MATCHES_PER_PAGE;return true;}
  if(matchCount>=MAX_CALLED_MATCHES)return false;
  CalledMatch &m=matches[matchCount++];m.table=table;m.p1=p1;m.p2=p2;
  m.status="called";m.matchId=matchId;m.matchType="Best of "+String((int)(item["bestOf"]|7));m.fromHub=true;
  matchPage=(matchCount-1)/MATCHES_PER_PAGE;
  return true;
}

void serviceHubAssignmentPolling(){
  if((screen!=SETUP&&screen!=GAMES_MENU)||WiFi.status()!=WL_CONNECTED||millis()-lastHubAssignmentPoll<3000)return;
  lastHubAssignmentPoll=millis();
  if(!hubConnected&& !probeHub())discoverHub();
  if(fetchHubAssignment(true))drawSetup();
}

void reportToHub(const String& status){
  if(WiFi.status()!=WL_CONNECTED||!hubIP.length()||selectedTable<1)return;
  int score1=leftPlayer==0?sideScore[0]:sideScore[1];
  int score2=leftPlayer==0?sideScore[1]:sideScore[0];
  JsonDocument doc;doc["table"]=selectedTable;doc["deviceId"]=WiFi.macAddress();doc["player1"]=player[0];doc["player2"]=player[1];
  doc["score1"]=score1;doc["score2"]=score2;doc["games1"]=gamesWon[0];doc["games2"]=gamesWon[1];doc["gameNumber"]=completedGames+1;doc["bestOf"]=bestOf;doc["status"]=status;
  doc["leftPlayer"]=leftPlayer;doc["timeoutLabel"]=status=="timeout"?timeoutLabel:"";doc["timeoutUsed1"]=playerTimeoutUsed[0];doc["timeoutUsed2"]=playerTimeoutUsed[1];
  JsonArray history=doc["gameScores"].to<JsonArray>();
  for(int g=0;g<completedGames;g++){JsonArray game=history.add<JsonArray>();game.add(gameScore[0][g]);game.add(gameScore[1][g]);}
  String body;serializeJson(doc,body);String url="http://"+hubIP+":"+String(SCOREKEEPER_HUB_PORT)+"/api/update";
  HTTPClient http;http.begin(url);http.setTimeout(900);http.addHeader("Content-Type","application/json");int code=http.POST(body);hubConnected=code>=200&&code<300;http.end();
}

void reportPhotonToHub(const String& status){
  if(WiFi.status()!=WL_CONNECTED||!hubIP.length())return;
  JsonDocument doc;doc["table"]=deviceTable;doc["deviceId"]=WiFi.macAddress();doc["gameType"]="photon_blitz";
  doc["player1"]=photonName[0];doc["player2"]=photonName[1];doc["score1"]=photonScore[0];doc["score2"]=photonScore[1];
  doc["leftPlayer"]=photonSidesSwapped?1:0;
  doc["games1"]=photonGamesWon[0];doc["games2"]=photonGamesWon[1];doc["gameNumber"]=photonGamesWon[0]+photonGamesWon[1]+1;doc["bestOf"]=3;
  doc["status"]=status;doc["durationSeconds"]=180;doc["remainingSeconds"]=(photonRemainingMs+999)/1000;
  doc["blitz"]=photonRemainingMs<=60000;doc["suddenDeath"]=photonSuddenDeath;
  doc["timeoutLabel"]=photonTimeoutActive?(photonTimeoutTeam>=0?fitName(photonName[photonTimeoutTeam],12)+" TIME":"REF TIME OUT"):"";doc["timeoutUsed1"]=photonTimeoutUsed[0];doc["timeoutUsed2"]=photonTimeoutUsed[1];
  JsonArray history=doc["gameScores"].to<JsonArray>();for(int g=0;g<photonCompletedGames;g++){JsonArray game=history.add<JsonArray>();game.add(photonGameScore[0][g]);game.add(photonGameScore[1][g]);}
  String body;serializeJson(doc,body);String url="http://"+hubIP+":"+String(SCOREKEEPER_HUB_PORT)+"/api/update";
  HTTPClient http;http.begin(url);http.setTimeout(900);http.addHeader("Content-Type","application/json");int code=http.POST(body);hubConnected=code>=200&&code<300;http.end();
}

bool fetchMatches(){
  if(WiFi.status()!=WL_CONNECTED)return false;
  bool tdLoaded=false;
  HTTPClient http; http.begin(DATA_URL); http.setFollowRedirects(HTTPC_STRICT_FOLLOW_REDIRECTS); http.setTimeout(15000);
  int code=http.GET();
  if(code==HTTP_CODE_OK){
    String payload=http.getString();JsonDocument doc;
    if(!deserializeJson(doc,payload)&&(doc["ok"]|false)){
      tdLoaded=true;matchCount=0;JsonArray called=doc["bracket"]["publicCalledMatches"].as<JsonArray>();
      for(JsonObject item:called){
        String status=String(item["status"]|"called");
        if(matchCount>=MAX_CALLED_MATCHES||status=="complete")continue;
        CalledMatch &m=matches[matchCount++];
        m.table=String(item["tableNumber"]|"");m.p1=String(item["p1"]|"TBD");m.p2=String(item["p2"]|"TBD");
        m.status=status;m.matchId=String(item["matchId"]|"");m.matchType=String(item["matchType"]|"");m.fromHub=false;
      }
    }
  }
  http.end();
  if(!hubConnected&&!probeHub())discoverHub();
  bool hubLoaded=fetchHubAssignment(false);
  matchPage=0;for(int i=0;i<matchCount;i++)if(matches[i].table.toInt()==deviceTable){matchPage=i/MATCHES_PER_PAGE;break;}
  return tdLoaded||hubLoaded;
}

void drawSetup(){
  screen=SETUP; tft.fillScreen(BG); header(WiFi.status()==WL_CONNECTED?"LIVE":"OFFLINE");
  drawWifiBars();
  drawStringButton(137,4,64,27,"T"+String(deviceTable)+" v",PANEL2,TEXT,1);
  tft.setTextDatum(TL_DATUM); tft.setTextColor(TEXT,BG);
  if(matchCount>0){
    if(lastStandardMatchAvailable)drawButton({4,41,122,25,"LAST SCORECARD",PANEL2,TEXT},1);
    else tft.drawString("TABLES",10,48,2);
    drawButton({130,41,84,25,"HISTORY",PANEL2,TEXT});
    drawButton({218,41,92,25,"+ MANUAL",LIME,BG});
  }else tft.drawString("CHOOSE YOUR TABLE",14,45,4);
  if(matchCount==0){
    tft.setTextDatum(MC_DATUM);tft.setTextColor(MUTED,BG);tft.drawString("No called matches found",160,105,2);
    drawButton({10,128,145,45,"MANUAL MATCH",LIME,BG});
    drawButton({165,128,145,45,"GAME HISTORY",PANEL2,TEXT});
    if(lastStandardMatchAvailable)drawButton({70,181,180,27,"LAST SCORECARD",PANEL2,TEXT},1);
  }else{
    int pages=max(1,(matchCount+MATCHES_PER_PAGE-1)/MATCHES_PER_PAGE);matchPage=constrain(matchPage,0,pages-1);
    int first=matchPage*MATCHES_PER_PAGE,shown=min(MATCHES_PER_PAGE,matchCount-first);
    for(int row=0;row<shown;row++){
      int i=first+row,y=73+row*35;bool assigned=matches[i].table.toInt()==deviceTable;uint16_t fill=assigned?PANEL2:PANEL;
      tft.fillRoundRect(10,y,300,31,5,fill);tft.drawRoundRect(10,y,300,31,5,assigned?LIME:LINE);
      tft.setTextDatum(ML_DATUM);tft.setTextColor(assigned?LIME:MUTED,fill);tft.drawString("T"+matches[i].table,18,y+15,2);
      tft.setTextColor(assigned?TEXT:MUTED,fill);String names=fitName(matches[i].p1,12)+"  vs  "+fitName(matches[i].p2,12);tft.drawString(names,55,y+15,2);
    }
    tft.setTextDatum(TR_DATUM);tft.setTextColor(MUTED,BG);tft.drawString(String(matchPage+1)+"/"+String(pages),306,66,1);
  }
  int pages=max(1,(matchCount+MATCHES_PER_PAGE-1)/MATCHES_PER_PAGE);
  drawButton({4,216,76,20,"WIFI",PANEL2,TEXT});drawButton({84,216,50,20,"PREV",matchPage>0?PANEL2:LINE,matchPage>0?TEXT:MUTED});
  drawButton({138,216,50,20,"NEXT",matchPage+1<pages?PANEL2:LINE,matchPage+1<pages?TEXT:MUTED});drawButton({192,216,124,20,"REFRESH",PANEL2,TEXT});
}

void openMatchCard(){
  if(WiFi.status()==WL_CONNECTED){
    tft.fillScreen(BG);header("REFRESHING MATCHES");tft.setTextDatum(MC_DATUM);tft.setTextColor(TEXT,BG);tft.drawString("Loading the next called match...",160,112,2);
    fetchMatches();
  }
  drawSetup();
}

String historyKey(int index){return "game"+String(index);}

void saveMatchHistory(){
  if(historySaved&&!editingCompletedMatch)return;
  if(!editingCompletedMatch){
    for(int i=min(historyCount,9);i>0;i--)preferences.putString(historyKey(i).c_str(),preferences.getString(historyKey(i-1).c_str(),""));
    historyCount=min(historyCount+1,10);preferences.putInt("historyCount",historyCount);
  }
  JsonDocument doc;
  struct tm clockTime;char dateText[24]="OFFLINE / NO DATE";
  if(getLocalTime(&clockTime,100))strftime(dateText,sizeof(dateText),"%Y-%m-%d %H:%M",&clockTime);
  doc["type"]="standard";doc["date"]=dateText;doc["table"]=selectedTable;doc["p1"]=player[0];doc["p2"]=player[1];doc["m1"]=gamesWon[0];doc["m2"]=gamesWon[1];
  JsonArray a=doc["g1"].to<JsonArray>();JsonArray b=doc["g2"].to<JsonArray>();
  for(int g=0;g<completedGames;g++){a.add(gameScore[0][g]);b.add(gameScore[1][g]);}
  String encoded;serializeJson(doc,encoded);preferences.putString("game0",encoded);historySaved=true;editingCompletedMatch=false;
}

void savePhotonHistory(){
  if(photonHistorySaved||!photonMatchComplete)return;photonHistorySaved=true;
  for(int i=min(historyCount,9);i>0;i--)preferences.putString(historyKey(i).c_str(),preferences.getString(historyKey(i-1).c_str(),""));
  JsonDocument doc;struct tm clockTime;char dateText[24]="OFFLINE / NO DATE";
  if(getLocalTime(&clockTime,100))strftime(dateText,sizeof(dateText),"%Y-%m-%d %H:%M",&clockTime);
  doc["type"]="photon_blitz";doc["date"]=dateText;doc["table"]=deviceTable;doc["p1"]=photonName[0];doc["p2"]=photonName[1];doc["m1"]=photonGamesWon[0];doc["m2"]=photonGamesWon[1];
  JsonArray a=doc["g1"].to<JsonArray>();JsonArray b=doc["g2"].to<JsonArray>();
  for(int g=0;g<photonCompletedGames;g++){a.add(photonGameScore[0][g]);b.add(photonGameScore[1][g]);}
  String encoded;serializeJson(doc,encoded);preferences.putString("game0",encoded);historyCount=min(historyCount+1,10);preferences.putInt("historyCount",historyCount);
}

bool loadHistory(int index,JsonDocument& doc){
  if(index<0||index>=historyCount)return false;String encoded=preferences.getString(historyKey(index).c_str(),"");return encoded.length()&&!deserializeJson(doc,encoded);
}

void drawHistoryList(){
  screen=HISTORY_LIST;tft.fillScreen(BG);header("GAME HISTORY");
  const char* filterNames[]={"ALL GAMES","STANDARD","PHOTON"};
  tft.setTextDatum(TL_DATUM);tft.setTextColor(TEXT,BG);tft.drawString("RECENT SCORECARDS",10,45,2);
  drawStringButton(202,39,108,23,filterNames[historyFilter],PANEL2,TEXT,1);
  int totalMatching=0;for(int i=0;i<historyCount;i++){JsonDocument doc;if(!loadHistory(i,doc))continue;String type=String(doc["type"]|"standard");if(historyFilter==0||(historyFilter==1&&type!="photon_blitz")||(historyFilter==2&&type=="photon_blitz"))totalMatching++;}
  historyPageCount=max(1,(totalMatching+3)/4);historyPage=constrain(historyPage,0,historyPageCount-1);historyVisibleCount=0;int skipped=0;
  for(int i=0;i<historyCount&&historyVisibleCount<4;i++){
    JsonDocument doc;if(!loadHistory(i,doc))continue;String type=String(doc["type"]|"standard");bool matches=historyFilter==0||(historyFilter==1&&type!="photon_blitz")||(historyFilter==2&&type=="photon_blitz");if(!matches)continue;
    if(skipped++<historyPage*4)continue;historyVisibleIndex[historyVisibleCount++]=i;
  }
  bool selectionVisible=false;for(int row=0;row<historyVisibleCount;row++)if(historyVisibleIndex[row]==selectedHistory)selectionVisible=true;if(!selectionVisible&&historyVisibleCount)selectedHistory=historyVisibleIndex[0];
  if(historyVisibleCount==0){tft.setTextDatum(MC_DATUM);tft.setTextColor(MUTED,BG);tft.drawString("No matching games saved",160,120,2);}
  for(int row=0;row<historyVisibleCount;row++){
    int i=historyVisibleIndex[row];JsonDocument doc;if(!loadHistory(i,doc))continue;int y=72+row*34;uint16_t fill=i==selectedHistory?PANEL2:PANEL;bool photon=String(doc["type"]|"standard")=="photon_blitz";uint16_t gameColor=photon?RED:TEXT;
    tft.fillRoundRect(10,y,300,30,5,fill);tft.drawRoundRect(10,y,300,30,5,i==selectedHistory?gameColor:LINE);
    tft.setTextDatum(ML_DATUM);tft.setTextColor(gameColor,fill);tft.drawString("T"+String(doc["table"]|0),17,y+10,1);
    String names=fitName(String(doc["p1"]|"Player 1"),9)+" vs "+fitName(String(doc["p2"]|"Player 2"),9);
    tft.setTextColor(gameColor,fill);tft.drawString(names,47,y+10,1);tft.setTextDatum(MR_DATUM);tft.drawString(String(doc["m1"]|0)+"-"+String(doc["m2"]|0),300,y+10,2);
    tft.setTextDatum(ML_DATUM);tft.setTextColor(MUTED,fill);tft.drawString(String(doc["date"]|"NO DATE"),47,y+22,1);
  }
  tft.setTextDatum(MC_DATUM);tft.setTextColor(MUTED,BG);tft.drawString(String(historyPage+1)+"/"+String(historyPageCount),160,207,1);
  drawButton({3,218,55,19,"BACK",PANEL2,TEXT});drawButton({62,218,54,19,"PREV",historyPage>0?PANEL2:LINE,historyPage>0?TEXT:MUTED});
  drawButton({120,218,54,19,"NEXT",historyPage+1<historyPageCount?PANEL2:LINE,historyPage+1<historyPageCount?TEXT:MUTED});drawButton({178,218,139,19,"VIEW SCORECARD",historyVisibleCount?LIME:LINE,historyVisibleCount?BG:MUTED},1);
}

void drawHistoryDetail(){
  JsonDocument doc;if(!loadHistory(selectedHistory,doc)){drawHistoryList();return;}
  bool photon=String(doc["type"]|"standard")=="photon_blitz";uint16_t gameColor=photon?RED:TEXT;
  screen=HISTORY_DETAIL;tft.fillScreen(BG);header(photon?"PHOTON SCORECARD":"STANDARD SCORECARD");
  tft.setTextDatum(MC_DATUM);tft.setTextColor(MUTED,BG);tft.drawString(String(doc["date"]|"NO DATE")+"  /  TABLE "+String(doc["table"]|0),160,48,1);
  tft.setTextColor(gameColor,BG);tft.drawString(fitName(String(doc["p1"]|"Player 1"),14)+"  vs  "+fitName(String(doc["p2"]|"Player 2"),14),160,72,4);
  String row1=String(doc["p1"]|"P1")+":";String row2=String(doc["p2"]|"P2")+":";
  for(JsonVariant value:doc["g1"].as<JsonArray>())row1+="  "+String(value.as<int>());
  for(JsonVariant value:doc["g2"].as<JsonArray>())row2+="  "+String(value.as<int>());
  tft.setTextColor(gameColor,BG);tft.drawString(row1,160,112,2);tft.drawString(row2,160,143,2);
  tft.setTextColor(gameColor,BG);tft.drawString("MATCH  "+String(doc["m1"]|0)+" - "+String(doc["m2"]|0),160,174,4);
  drawButton({74,207,172,27,"BACK TO LIST",PANEL2,TEXT});
}

void drawManualMatch(){
  screen=MANUAL_MATCH;tft.fillScreen(BG);header("MANUAL MATCH");
  drawWifiBars();
  drawStringButton(126,4,76,27,"T"+String(deviceTable)+" v",PANEL2,TEXT,1);
  tft.setTextDatum(MC_DATUM);tft.setTextColor(MUTED,BG);tft.drawString("USES DEFAULT TABLE "+String(deviceTable),160,45,2);
  tft.setTextColor(TEXT,BG);tft.drawString("PLAYER 1  vs  PLAYER 2",160,67,4);
  tft.setTextColor(MUTED,BG);tft.drawString("CHOOSE MATCH LENGTH",160,91,2);
  drawButton({14,111,91,54,"BEST OF 3",PANEL,TEXT});
  drawButton({114,111,91,54,"BEST OF 5",LIME,BG});
  drawButton({214,111,91,54,"BEST OF 7",PANEL,TEXT});
  drawButton({8,193,96,32,"WIFI",PANEL2,TEXT});
  drawButton({112,193,96,32,"CHOOSE GAME",ORANGE,TEXT},1);
  drawButton({216,193,96,32,"MENU",PANEL2,TEXT});
}

void beginTableChange(bool fromManual){
  tableChangeFromManual=fromManual;tablePin="";tablePinMessage="";
  screen=TABLE_PIN;tft.fillScreen(BG);header("TABLE LOCK");
  tft.setTextDatum(MC_DATUM);tft.setTextColor(TEXT,BG);tft.drawString("ENTER TABLE CHANGE CODE",160,48,2);
  tft.setTextColor(LIME,BG);tft.drawString("____",160,70,4);
  const char* keys[]={"1","2","3","4","5","6","7","8","9","DEL","0","CANCEL"};
  for(int i=0;i<12;i++){int col=i%3,row=i/3;drawButton({31+col*87,91+row*31,82,27,keys[i],i==9?ORANGE:(i==11?PANEL2:PANEL),TEXT},1);}
}

void drawTablePin(){
  screen=TABLE_PIN;tft.fillScreen(BG);header("TABLE LOCK");
  tft.setTextDatum(MC_DATUM);tft.setTextColor(TEXT,BG);tft.drawString("ENTER TABLE CHANGE CODE",160,48,2);
  String shown="";for(unsigned int i=0;i<tablePin.length();i++)shown+="*";while(shown.length()<4)shown+="_";
  tft.setTextColor(tablePinMessage.length()?RED:LIME,BG);tft.drawString(tablePinMessage.length()?tablePinMessage:shown,160,70,tablePinMessage.length()?2:4);
  const char* keys[]={"1","2","3","4","5","6","7","8","9","DEL","0","CANCEL"};
  for(int i=0;i<12;i++){int col=i%3,row=i/3;drawButton({31+col*87,91+row*31,82,27,keys[i],i==9?ORANGE:(i==11?PANEL2:PANEL),TEXT},1);}
}

void drawTableSelect(){
  screen=TABLE_SELECT;tft.fillScreen(BG);header("SELECT DEFAULT TABLE");
  tft.setTextDatum(MC_DATUM);tft.setTextColor(MUTED,BG);tft.drawString("TAP NEW DEVICE TABLE",160,43,2);
  for(int table=1;table<=30;table++){
    int index=table-1,col=index%6,row=index/6;uint16_t fill=table==deviceTable?LIME:PANEL;
    drawStringButton(4+col*53,60+row*29,49,25,"T"+String(table),fill,table==deviceTable?BG:TEXT,1);
  }
  drawButton({110,211,100,24,"CANCEL",PANEL2,TEXT},1);
}

void saveDefaultTable(int table){
  deviceTable=constrain(table,1,30);if(venueTableCount<deviceTable)venueTableCount=deviceTable;
  preferences.putInt("deviceTable",deviceTable);preferences.putInt("tableCount",venueTableCount);
  matchPage=0;for(int i=0;i<matchCount;i++)if(matches[i].table.toInt()==deviceTable){matchPage=i/MATCHES_PER_PAGE;break;}
  if(tableChangeFromManual)drawManualMatch();else drawSetup();
}

void drawManualTableSetup(){
  screen=MANUAL_TABLES;tft.fillScreen(BG);header("TABLE SETUP");
  tft.setTextDatum(MC_DATUM);tft.setTextColor(TEXT,BG);tft.drawString("MANUAL MATCH TABLE",160,48,4);
  tft.setTextColor(MUTED,BG);tft.drawString("TABLES AT THIS EVENT",160,78,2);
  drawButton({35,92,55,40,"-",PANEL2,TEXT},4);drawStringButton(100,92,120,40,String(venueTableCount),PANEL,TEXT,4);drawButton({230,92,55,40,"+",PANEL2,TEXT},4);
  tft.setTextColor(MUTED,BG);tft.drawString("THIS DEVICE DEFAULTS TO",160,148,2);
  drawButton({35,162,55,40,"-",PANEL2,TEXT},4);drawStringButton(100,162,120,40,"TABLE "+String(deviceTable),PANEL,TEXT,2);drawButton({230,162,55,40,"+",PANEL2,TEXT},4);
  drawButton({74,211,172,24,"CONTINUE  >",LIME,BG});
}

void drawManualNames(){
  screen=MANUAL_NAMES;tft.fillScreen(BG);header("MANUAL PLAYERS");
  tft.setTextDatum(MC_DATUM);tft.setTextColor(MUTED,BG);tft.drawString(editingPlayer==0?"ENTER PLAYER 1 NAME":"ENTER PLAYER 2 NAME",160,43,2);
  tft.setTextColor(TEXT,BG);tft.drawString(fitName(player[editingPlayer],24),160,62,4);
  const int lengths[]={10,9,manualKeyboardMode==2?8:7};const int starts[]={4,19,manualKeyboardMode==2?35:50};
  for(int row=0;row<3;row++)for(int col=0;col<lengths[row];col++){
    char label[2]={manualKey(row,col),0};drawButton({starts[row]+col*31,79+row*32,28,28,label,PANEL,TEXT},1);
  }
  drawButton({4,177,48,27,"SHIFT",PANEL2,TEXT},1);drawButton({56,177,48,27,manualKeyboardMode==2?"ABC":"123",PANEL2,TEXT},1);
  drawButton({108,177,74,27,"SPACE",PANEL2,TEXT},1);drawButton({186,177,48,27,"DEL",ORANGE,TEXT},1);drawButton({238,177,78,27,"DEFAULT",PANEL2,TEXT},1);
  drawButton({4,211,74,24,"BACK",PANEL2,TEXT},1);drawButton({82,211,234,24,editingPlayer==0?"NEXT":"START",LIME,BG},1);
}

void beginManualScoring(){
  if(!player[0].length())player[0]="Player 1";if(!player[1].length())player[1]="Player 2";
  leftPlayer=0;sideScore[0]=sideScore[1]=gamesWon[0]=gamesWon[1]=completedGames=0;playerTimeoutUsed[0]=playerTimeoutUsed[1]=false;historySaved=false;editingCompletedMatch=false;lastStandardMatchAvailable=false;memset(gameScore,0,sizeof(gameScore));drawScore();reportToHub("playing");
}

void startManualMatch(int format){
  player[0]="Player 1";player[1]="Player 2";manualNameEdited[0]=manualNameEdited[1]=false;
  selectedTable=deviceTable;bestOf=format;isManualMatch=true;editingPlayer=0;manualKeyboardMode=0;drawManualNames();
}

void scanWifi(){
  tft.fillScreen(BG);header("WIFI SETTINGS");tft.setTextDatum(MC_DATUM);tft.setTextColor(TEXT,BG);tft.drawString("Scanning for networks...",160,120,2);
  WiFi.mode(WIFI_STA);
  int found=WiFi.scanNetworks(false,true);
  networkCount=0;wifiPage=0;
  for(int i=0;i<savedNetworkCount&&networkCount<MAX_WIFI_NETWORKS;i++){
    networkName[networkCount]=savedSSID[i];networkOpen[networkCount]=false;
    networkRSSI[networkCount]=-99;networkSaved[networkCount]=true;networkCount++;
  }
  if(found<0){wifiMessage="SCAN FAILED - TAP SCAN";WiFi.scanDelete();return;}
  for(int i=0;i<found&&networkCount<MAX_WIFI_NETWORKS;i++){
    String ssid=WiFi.SSID(i);ssid.trim();if(!ssid.length())continue;
    int duplicate=-1;for(int n=0;n<networkCount;n++)if(networkName[n]==ssid){duplicate=n;break;}
    if(duplicate>=0){networkRSSI[duplicate]=WiFi.RSSI(i);networkOpen[duplicate]=(WiFi.encryptionType(i)==WIFI_AUTH_OPEN);continue;}
    networkName[networkCount]=ssid;
    networkOpen[networkCount]=(WiFi.encryptionType(i)==WIFI_AUTH_OPEN);
    networkRSSI[networkCount]=WiFi.RSSI(i);
    networkSaved[networkCount]=false;
    networkCount++;
  }
  wifiMessage=networkCount?String(networkCount)+" NETWORKS FOUND":"NO NETWORKS FOUND";
  WiFi.scanDelete();
}

void drawWifiList(){
  screen=WIFI_LIST;tft.fillScreen(BG);header("WIFI SETTINGS");
  drawWifiBars();
  tft.setTextDatum(TL_DATUM);tft.setTextColor(TEXT,BG);tft.drawString("CHOOSE A NETWORK",10,44,2);
  drawStringButton(190,39,120,23,autoWifiEnabled?"AUTO WIFI: ON":"AUTO WIFI: OFF",autoWifiEnabled?LIME:PANEL2,autoWifiEnabled?BG:TEXT,1);
  int pageCount=max(1,(networkCount+WIFI_NETWORKS_PER_PAGE-1)/WIFI_NETWORKS_PER_PAGE);
  wifiPage=constrain(wifiPage,0,pageCount-1);
  tft.setTextDatum(TR_DATUM);tft.setTextColor(MUTED,BG);tft.drawString(String(wifiPage+1)+"/"+String(pageCount),182,48,1);
  int first=wifiPage*WIFI_NETWORKS_PER_PAGE;
  int shown=min(WIFI_NETWORKS_PER_PAGE,networkCount-first);
  if(networkCount==0){tft.setTextDatum(MC_DATUM);tft.setTextColor(MUTED,BG);tft.drawString("No networks found - tap SCAN",160,112,2);}
  for(int row=0;row<shown;row++){
    int i=first+row,y=65+row*34;
    drawStringButton(10,y,networkSaved[i]?220:300,30,fitName(networkName[i],networkSaved[i]?17:23),PANEL,TEXT);
    if(networkSaved[i])drawStringButton(234,y,76,30,"FORGET",ORANGE,TEXT,1);
    tft.setTextDatum(MR_DATUM);tft.setTextColor(networkOpen[i]?LIME:MUTED,PANEL);
    String detail=networkSaved[i]?"SAVED":(networkOpen[i]?"OPEN":"LOCK");
    if(networkRSSI[i]>-99)detail=String(networkRSSI[i])+"  "+detail;
    if(!networkSaved[i])tft.drawString(detail,300,y+15,1);
  }
  tft.setTextDatum(MC_DATUM);tft.setTextColor(WiFi.status()==WL_CONNECTED?LIME:MUTED,BG);
  String status=wifiMessage.length()?wifiMessage:(WiFi.status()==WL_CONNECTED?"CONNECTED: "+fitName(WiFi.SSID(),18):"NOT CONNECTED");
  tft.drawString(status,160,204,1);
  drawButton({3,218,55,19,"BACK",PANEL2,TEXT});drawButton({62,218,54,19,"PREV",wifiPage>0?PANEL2:LINE,wifiPage>0?TEXT:MUTED});
  drawButton({120,218,54,19,"NEXT",wifiPage+1<pageCount?PANEL2:LINE,wifiPage+1<pageCount?TEXT:MUTED});
  drawButton({178,218,75,19,"HUB",LIME,BG});drawButton({257,218,60,19,"SCAN",PANEL2,TEXT});
}

void drawHubSetup(){
  screen=HUB_SETUP;tft.fillScreen(BG);header("HUB SERVER");
  tft.setTextDatum(MC_DATUM);tft.setTextColor(MUTED,BG);tft.drawString("PI IP ADDRESS  /  PORT 8081",160,46,2);
  tft.setTextColor(TEXT,BG);tft.drawString(hubIP.length()?hubIP:"0.0.0.0",160,68,4);
  const char* keys[]={"1","2","3","4","5","6","7","8","9",".","0","DEL"};
  for(int i=0;i<12;i++){int col=i%3,row=i/3;drawButton({8+col*57,88+row*34,52,30,keys[i],i==11?ORANGE:PANEL,i==11?TEXT:TEXT},1);}
  drawButton({190,88,122,38,"TEST",BLUE,TEXT});drawButton({190,134,122,38,"SAVE",LIME,BG});drawButton({190,180,122,38,"BACK",PANEL2,TEXT});
  tft.setTextColor(hubTestMessage.startsWith("CONNECTED")?LIME:(hubTestMessage.length()?RED:MUTED),BG);tft.drawString(hubTestMessage,160,229,1);
}

bool probeHub(){
  if(WiFi.status()!=WL_CONNECTED||!hubIP.length()){hubConnected=false;return false;}
  String url="http://"+hubIP+":"+String(SCOREKEEPER_HUB_PORT)+"/api/health";HTTPClient http;http.begin(url);http.setTimeout(2500);
  int code=http.GET();String response=code==200?http.getString():"";http.end();hubConnected=code==200&&response.indexOf("scorekeeper-hub")>=0;return hubConnected;
}

void testHubConnection(){
  if(WiFi.status()!=WL_CONNECTED){hubConnected=false;hubTestMessage="NO WIFI CONNECTION";drawHubSetup();return;}
  hubTestMessage=probeHub()?"CONNECTED TO SCOREKEEPER HUB":"HUB NOT REACHABLE";drawHubSetup();
}

char keyboardKey(int mode,int row,int col){
  const char* upper[]={"QWERTYUIOP","ASDFGHJKL","ZXCVBNM"};
  const char* lower[]={"qwertyuiop","asdfghjkl","zxcvbnm"};
  const char* symbol[]={"1234567890","!@#$%^&*()","-_=+.,?/"};
  const int lengths[]={10,9,7};
  int length=mode==2?(row==2?8:10):lengths[row];
  if(row<0||row>2||col<0||col>=length)return 0;
  return mode==0?upper[row][col]:(mode==1?lower[row][col]:symbol[row][col]);
}

char wifiKey(int row,int col){return keyboardKey(keyboardMode,row,col);}
char manualKey(int row,int col){return keyboardKey(manualKeyboardMode,row,col);}
char photonKey(int row,int col){return keyboardKey(photonKeyboardMode,row,col);}

void drawWifiPassword(){
  screen=WIFI_PASSWORD;tft.fillScreen(BG);header("WIFI PASSWORD");
  tft.setTextDatum(MC_DATUM);tft.setTextColor(TEXT,BG);tft.drawString(fitName(pendingSSID,25),160,46,2);
  String shownPassword=pendingPassword;
  if(shownPassword.length()>28)shownPassword="..."+shownPassword.substring(shownPassword.length()-25);
  tft.setTextColor(LIME,BG);tft.drawString(shownPassword.length()?shownPassword:"ENTER PASSWORD",160,65,2);
  const int lengths[]={10,9,keyboardMode==2?8:7};const int starts[]={4,19,keyboardMode==2?35:50};
  for(int row=0;row<3;row++)for(int col=0;col<lengths[row];col++)
    drawStringButton(starts[row]+col*31,78+row*31,28,27,String(wifiKey(row,col)),PANEL,TEXT,1);
  drawButton({4,173,54,26,"SHIFT",PANEL2,TEXT},1);drawButton({62,173,54,26,"123",PANEL2,TEXT},1);
  drawButton({120,173,104,26,"SPACE",PANEL,TEXT},1);drawButton({228,173,88,26,"DEL",ORANGE,TEXT},1);
  drawButton({4,207,104,27,"BACK",PANEL2,TEXT},1);drawButton({112,207,204,27,"JOIN NETWORK",LIME,BG},1);
}

void connectPendingWifi(){
  autoWifiTrying=false;
  tft.fillScreen(BG);header("CONNECTING");tft.setTextDatum(MC_DATUM);tft.setTextColor(TEXT,BG);tft.drawString("Connecting to "+fitName(pendingSSID,20),160,105,2);
  WiFi.disconnect(true);delay(250);WiFi.mode(WIFI_STA);WiFi.begin(pendingSSID.c_str(),pendingPassword.c_str());
  for(int i=0;i<30&&WiFi.status()!=WL_CONNECTED;i++)delay(500);
  if(WiFi.status()==WL_CONNECTED){
    rememberNetwork(pendingSSID,pendingPassword);
    configTzTime(TZ_INFO,"pool.ntp.org","time.google.com");
    if(!probeHub())discoverHub();fetchMatches();wifiMessage="CONNECTED: "+fitName(pendingSSID,18);scanWifi();wifiMessage="CONNECTED: "+fitName(pendingSSID,18);drawWifiList();
  }else{
    tft.setTextColor(RED,BG);tft.drawString("Connection failed",160,135,2);delay(1500);drawWifiPassword();
  }
}

void drawGamesMenu(){
  screen=GAMES_MENU;tft.fillScreen(BG);header("AIR HOCKEY GAMES");drawWifiBars();
  tft.setTextDatum(MC_DATUM);tft.setTextColor(TEXT,BG);tft.drawString("CHOOSE A GAME",160,48,4);
  tft.fillRoundRect(10,70,145,104,9,PANEL);tft.drawRoundRect(10,70,145,104,9,LIME);
  tft.setTextColor(LIME,PANEL);tft.drawString("STANDARD",82,96,4);tft.drawString("MATCH",82,121,4);
  tft.setTextColor(MUTED,PANEL);tft.drawString("BEST OF 3 / 5 / 7",82,153,1);
  tft.fillRoundRect(165,70,145,104,9,PANEL);tft.drawRoundRect(165,70,145,104,9,ORANGE);
  tft.setTextColor(ORANGE,PANEL);tft.drawString("PHOTON",237,96,4);tft.drawString("BLITZ",237,121,4);
  tft.setTextColor(MUTED,PANEL);tft.drawString("3 MIN / FINAL 1 MIN 2X",237,153,1);
  drawButton({10,194,145,34,"WIFI",PANEL2,TEXT});drawButton({165,194,145,34,"FULL MENU",PANEL2,TEXT});
}

void drawPhotonNames(){
  screen=PHOTON_NAMES;tft.fillScreen(BG);header("PHOTON TEAMS");
  tft.setTextDatum(MC_DATUM);tft.setTextColor(photonEditingTeam==0?RED:BLUE,BG);tft.drawString(photonEditingTeam==0?"EDIT RED TEAM":"EDIT BLUE TEAM",160,43,2);
  tft.setTextColor(TEXT,BG);tft.drawString(fitName(photonName[photonEditingTeam],20),160,62,4);
  const int lengths[]={10,9,photonKeyboardMode==2?8:7};const int starts[]={4,19,photonKeyboardMode==2?35:50};
  for(int row=0;row<3;row++)for(int col=0;col<lengths[row];col++){
    char label[2]={photonKey(row,col),0};drawButton({starts[row]+col*31,79+row*32,28,28,label,PANEL,TEXT},1);
  }
  drawButton({4,177,48,27,"SHIFT",PANEL2,TEXT},1);drawButton({56,177,48,27,photonKeyboardMode==2?"ABC":"123",PANEL2,TEXT},1);
  drawButton({108,177,74,27,"SPACE",PANEL2,TEXT},1);drawButton({186,177,48,27,"DEL",ORANGE,TEXT},1);drawButton({238,177,78,27,"DEFAULT",PANEL2,TEXT},1);
  drawButton({4,211,74,24,"BACK",PANEL2,TEXT},1);drawButton({82,211,234,24,photonEditingTeam==0?"NEXT":"START",LIME,BG},1);
}

void beginPhotonNames(){
  photonName[0]="RED TEAM";photonName[1]="BLUE TEAM";photonNameEdited[0]=photonNameEdited[1]=false;photonEditingTeam=0;photonKeyboardMode=0;drawPhotonNames();
}

void drawSkull(int cx,int cy,int scale,uint16_t color){
  int r=7*scale;tft.drawCircle(cx,cy-r/3,r,color);tft.fillRect(cx-r/2,cy+r/3,r,4*scale,color);
  tft.fillCircle(cx-3*scale,cy-r/2,2*scale,BG);tft.fillCircle(cx+3*scale,cy-r/2,2*scale,BG);tft.fillTriangle(cx,cy,cx-2*scale,cy+3*scale,cx+2*scale,cy+3*scale,BG);
  for(int o=-scale;o<=scale;o++){tft.drawLine(cx-r,cy+r+o,cx+r,cy+2*r+o,color);tft.drawLine(cx+r,cy+r+o,cx-r,cy+2*r+o,color);}
}

void flashSuddenDeath(){
  tft.fillScreen(BG);header("SUDDEN DEATH");drawSkull(160,105,4,RED);tft.setTextDatum(MC_DATUM);tft.setTextColor(RED,BG);tft.drawString("NEXT POINT WINS",160,190,4);delay(1000);
}

void drawPhotonClock(){
  if(screen!=PHOTON_GAME)return;
  int tenths=(photonRemainingMs+99)/100,seconds=tenths/10;
  tft.fillRect(70,38,180,40,BG);tft.setTextDatum(MC_DATUM);
  tft.setTextColor(photonSuddenDeath?RED:(photonRemainingMs<=60000?ORANGE:TEXT),BG);
  String phase=photonTimeoutActive?"TIMEOUT":(photonSuddenDeath?"SUDDEN DEATH - NEXT POINT WINS":(photonRemainingMs<=60000?"BLITZ  2X POINTS":"REGULATION  1 POINT"));
  tft.drawString(phase,160,44,1);
  char clockText[8];
  if(photonTimeoutActive){int timeoutLeft=max(0,(int)((photonTimeoutEnds-millis()+999)/1000));snprintf(clockText,sizeof(clockText),"T:%02d",timeoutLeft);}
  else snprintf(clockText,sizeof(clockText),"%d:%02d.%d",seconds/60,seconds%60,tenths%10);
  tft.drawString(clockText,160,62,4);
  if(photonSuddenDeath)drawSkull(66,58,1,RED);
}

void drawPhotonGame(){
  screen=PHOTON_GAME;tft.fillScreen(BG);header("PHOTON BLITZ");drawWifiBars();drawSwapIconButton();drawPhotonClock();drawRefTimeoutIcon();
  uint16_t colors[]={RED,BLUE};
  for(int visualSide=0;visualSide<2;visualSide++){
    int team=photonSidesSwapped?1-visualSide:visualSide,x=visualSide?164:4;
    tft.fillRoundRect(x,79,152,91,8,PANEL);tft.drawRoundRect(x,79,152,91,8,colors[team]);
    tft.setTextDatum(MC_DATUM);tft.setTextColor(colors[team],PANEL);tft.drawString(fitName(photonName[team],14),x+76,92,2);
    tft.setTextColor(TEXT,PANEL);tft.drawNumber(photonScore[team],x+76,133,7);
  }
  drawButton({4,173,104,27,"-",PANEL2,TEXT},2);drawButton({212,173,104,27,"-",PANEL2,TEXT},2);
  drawButton({112,173,96,63,photonRunning?"PAUSE":"START / RESUME",photonRunning?ORANGE:LIME,photonRunning?TEXT:BG},1);
  int leftTeam=photonSidesSwapped?1:0,rightTeam=1-leftTeam;
  drawStringButton(4,204,104,32,photonTimeoutUsed[leftTeam]?"TIME USED":fitName(photonName[leftTeam],7)+" TIME",photonTimeoutUsed[leftTeam]?LINE:colors[leftTeam],photonTimeoutUsed[leftTeam]?MUTED:TEXT,1);
  drawStringButton(212,204,104,32,photonTimeoutUsed[rightTeam]?"TIME USED":fitName(photonName[rightTeam],7)+" TIME",photonTimeoutUsed[rightTeam]?LINE:colors[rightTeam],photonTimeoutUsed[rightTeam]?MUTED:TEXT,1);
}

void startPhotonGame(){
  photonScore[0]=photonScore[1]=photonGoalCount[0]=photonGoalCount[1]=0;memset(photonGoalValue,0,sizeof(photonGoalValue));
  photonRemainingMs=180000;photonRunning=false;photonBlitzReported=false;photonSuddenDeath=false;photonTimeoutActive=false;photonTimeoutTeam=-1;photonTimeoutUsed[0]=photonTimeoutUsed[1]=false;drawPhotonGame();reportPhotonToHub("ready");
}

void startPhotonMatch(){
  photonGamesWon[0]=photonGamesWon[1]=0;photonCompletedGames=0;photonMatchComplete=false;photonHistorySaved=false;photonSidesSwapped=false;memset(photonGameScore,0,sizeof(photonGameScore));startPhotonGame();
}

void addPhotonGoal(int side){
  if(photonTimeoutActive)return;
  if(photonRunning)photonRunning=false;
  int value=photonRemainingMs<=60000?2:1;photonScore[side]+=value;
  if(photonGoalCount[side]<30)photonGoalValue[side][photonGoalCount[side]++]=value;
  if(photonSuddenDeath){finishPhotonGame();return;}
  drawPhotonGame();reportPhotonToHub("paused");
}

void removePhotonGoal(int side){
  if(photonGoalCount[side]<=0)return;
  photonScore[side]-=photonGoalValue[side][--photonGoalCount[side]];drawPhotonGame();reportPhotonToHub("playing");
}

void finishPhotonGame(){
  photonRunning=false;photonRemainingMs=0;int winner=-1;if(photonScore[0]>photonScore[1])winner=0;else if(photonScore[1]>photonScore[0])winner=1;
  if(photonCompletedGames<3){photonGameScore[0][photonCompletedGames]=photonScore[0];photonGameScore[1][photonCompletedGames]=photonScore[1];photonCompletedGames++;}
  if(winner>=0)photonGamesWon[winner]++;photonMatchComplete=photonGamesWon[0]>=2||photonGamesWon[1]>=2;
  if(photonMatchComplete)savePhotonHistory();reportPhotonToHub(photonMatchComplete?"complete":"between_games");screen=PHOTON_RESULT;tft.fillScreen(BG);header(photonMatchComplete?"PHOTON MATCH COMPLETE":"PHOTON GAME COMPLETE");
  tft.setTextDatum(MC_DATUM);tft.setTextColor(ORANGE,BG);String winnerText=winner<0?"TIE GAME":fitName(photonName[winner],16)+" WINS";tft.drawString(winnerText,160,50,4);
  if(photonMatchComplete){
    for(int team=0;team<2;team++){
      String row=fitName(photonName[team],10)+":";for(int g=0;g<photonCompletedGames;g++)row+="  G"+String(g+1)+" "+String(photonGameScore[team][g]);
      tft.setTextColor(team==winner?ORANGE:TEXT,BG);tft.drawString(row,160,91+team*34,2);
    }
    tft.setTextColor(MUTED,BG);tft.drawString("MATCH  "+String(photonGamesWon[0])+" - "+String(photonGamesWon[1]),160,163,4);
  }else{
    tft.setTextColor(TEXT,BG);tft.drawString(String(photonScore[0])+"  -  "+String(photonScore[1]),160,112,7);
    tft.setTextColor(MUTED,BG);tft.drawString("MATCH  "+String(photonGamesWon[0])+" - "+String(photonGamesWon[1])+"  /  FIRST TO 2",160,163,2);
  }
  drawButton({14,194,142,34,"GAMES MENU",PANEL2,TEXT});drawButton({164,194,142,34,photonMatchComplete?"NEW MATCH":"NEXT GAME",LIME,BG});
}

void updatePhotonGame(){
  if(screen!=PHOTON_GAME)return;
  if(photonTimeoutActive){
    if((long)(millis()-photonTimeoutEnds)>=0){photonTimeoutActive=false;drawPhotonGame();reportPhotonToHub("paused");}
    else drawPhotonClock();
    return;
  }
  if(!photonRunning)return;
  unsigned long now=millis(),elapsed=now-photonLastTick;photonLastTick=now;
  photonRemainingMs=elapsed>=photonRemainingMs?0:photonRemainingMs-elapsed;
  if(!photonBlitzReported&&photonRemainingMs<=60000){photonBlitzReported=true;reportPhotonToHub("blitz");}
  static int lastShown=-1;int shown=(photonRemainingMs+99)/100;
  if(shown!=lastShown){lastShown=shown;drawPhotonClock();}
  if(photonRemainingMs==0){
    photonRunning=false;
    if(photonScore[0]==photonScore[1]){photonSuddenDeath=true;flashSuddenDeath();drawPhotonGame();reportPhotonToHub("sudden_death");}
    else finishPhotonGame();
  }
}

void drawInitials(){
  screen=INITIALS; tft.fillScreen(BG); header(editingPlayer==0?"PLAYER 1":"PLAYER 2");
  tft.setTextDatum(MC_DATUM); tft.setTextColor(MUTED,BG); tft.drawString("ENTER UP TO 3 INITIALS",160,48,2);
  String shown=player[editingPlayer]; while(shown.length()<3) shown+="_";
  tft.setTextColor(TEXT,BG); tft.drawString(shown,160,73,4);
  const char keys[]="ABCDEFGHIJKLMNOPQRSTUVWXYZ<*";
  for(int i=0;i<28;i++){
    int col=i%7,row=i/7,x=8+col*44,y=91+row*35;
    char label[5]={keys[i],0,0,0,0};
    if(keys[i]=='<') strcpy(label,"DEL");
    if(keys[i]=='*') strcpy(label,"OK");
    uint16_t fill=keys[i]=='*'?LIME:(keys[i]=='<'?ORANGE:PANEL);
    drawButton({x,y,40,31,label,fill,keys[i]=='*'?BG:TEXT});
  }
}

String sideName(int side){ int id=(side==0)?leftPlayer:1-leftPlayer; return player[id]; }
int sidePlayer(int side){ return side==0?leftPlayer:1-leftPlayer; }

void scorePanel(int side,int x){
  int id=sidePlayer(side);
  uint16_t sideColor=id==0?RED:BLUE;
  tft.fillRoundRect(x,53,143,94,9,PANEL); tft.drawRoundRect(x,53,143,94,9,sideColor);
  tft.setTextDatum(MC_DATUM); tft.setTextColor(MUTED,PANEL);
  String line=player[id]+"  GAMES "+gamesWon[id]; tft.drawString(line,x+71,68,2);
  tft.setTextColor(TEXT,PANEL); tft.drawNumber(sideScore[side],x+71,113,7);
  drawButton({x+52,153,39,27,"-",PANEL2,TEXT},2);
}

void drawScore(){
  screen=SCORE; tft.fillScreen(BG); header("GAME IN PROGRESS");
  drawSwapIconButton();
  tft.setTextDatum(MC_DATUM); tft.setTextColor(MUTED,BG);
  String sub="TABLE "+String(selectedTable)+"  /  GAME "+String(completedGames+1)+"  /  FIRST TO 7"; tft.drawString(sub,160,44,2);
  scorePanel(0,10); scorePanel(1,167);
  int leftId=sidePlayer(0),rightId=sidePlayer(1);
  drawStringButton(5,188,76,47,playerTimeoutUsed[leftId]?"USED":"TIME OUT",playerTimeoutUsed[leftId]?LINE:RED,playerTimeoutUsed[leftId]?MUTED:TEXT,2);
  drawButton({88,188,144,47,"REF TIME OUT",PANEL2,TEXT});
  drawStringButton(239,188,76,47,playerTimeoutUsed[rightId]?"USED":"TIME OUT",playerTimeoutUsed[rightId]?LINE:BLUE,playerTimeoutUsed[rightId]?MUTED:TEXT,2);
}

void drawGameConfirm(){
  screen=CONFIRM_GAME; tft.fillScreen(BG); header("CHECK GAME SCORE");
  tft.setTextDatum(MC_DATUM); tft.setTextColor(LIME,BG); tft.drawString("GAME COMPLETE",160,55,2);
  String result=sideName(0)+"  "+sideScore[0]+"  -  "+sideScore[1]+"  "+sideName(1);
  tft.setTextColor(TEXT,BG); tft.drawString(result,160,92,4);
  tft.setTextColor(MUTED,BG); tft.drawString("Is this score correct?",160,126,2);
  drawButton({14,151,140,62,"NO, EDIT",PANEL2,TEXT},2);
  drawButton({166,151,140,62,"YES, SAVE",LIME,BG},2);
}

void saveGame(){
  int left=leftPlayer,right=1-leftPlayer;
  gameScore[left][completedGames]=sideScore[0]; gameScore[right][completedGames]=sideScore[1];
  if(sideScore[0]>sideScore[1]) gamesWon[left]++; else gamesWon[right]++;
  completedGames++;
  int needed=bestOf/2+1;
  if(gamesWon[0]>=needed||gamesWon[1]>=needed){drawResult();return;}
  reportToHub("between_games");
  screen=SWITCH_SIDES; tft.fillScreen(BG); header("BETWEEN GAMES");
  tft.setTextDatum(MC_DATUM); tft.setTextColor(ORANGE,BG); tft.drawString("SWITCH SIDES",160,70,4);
  tft.setTextColor(TEXT,BG); tft.drawString(player[left]+"   <-->   "+player[right],160,111,2);
  tft.setTextColor(MUTED,BG); tft.drawString("Scoring controls switch with the players",160,139,2);
  drawButton({44,174,232,46,"SIDES SWITCHED  >",LIME,BG});
}

void startTimeout(const String& label){timeoutLabel=label;timeoutEnds=millis()+10000UL;lastTimeoutSecond=-1;screen=TIMEOUT_SCREEN;tft.fillScreen(BG);header("TIME OUT");reportToHub("timeout");}
void updateTimeout(){
  long remaining=(long)(timeoutEnds-millis());
  if(remaining<=0){drawScore();reportToHub("playing");return;}
  int seconds=(remaining+999)/1000; if(seconds==lastTimeoutSecond)return; lastTimeoutSecond=seconds;
  tft.fillRect(0,36,320,204,BG); tft.setTextDatum(MC_DATUM); tft.setTextColor(ORANGE,BG); tft.drawString(timeoutLabel,160,69,4);
  tft.setTextColor(TEXT,BG); tft.drawNumber(seconds,160,125,7); tft.setTextColor(MUTED,BG); tft.drawString("Play resumes automatically",160,174,2);
  drawButton({74,196,172,34,"RESUME PLAY",LIME,BG});
}

void drawResultCard(){
  screen=RESULT; tft.fillScreen(BG); header("SCORECARD OUTPUT");
  int winner=gamesWon[0]>gamesWon[1]?0:1;
  tft.setTextDatum(MC_DATUM); tft.setTextColor(LIME,BG); tft.drawString(player[winner]+" WINS",160,49,4);
  String scorecardTitle="TABLE "+String(selectedTable)+" - ENTER THESE SCORES";
  tft.setTextColor(MUTED,BG); tft.drawString(scorecardTitle,160,73,2);
  for(int p=0;p<2;p++){
    String row=player[p]+":"; for(int g=0;g<completedGames;g++) row+="  "+String(gameScore[p][g]);
    tft.setTextColor(p==winner?LIME:TEXT,BG); tft.drawString(row,160,105+p*35,2);
  }
  String matchLine="MATCH  "+String(gamesWon[0])+" - "+String(gamesWon[1]);
  tft.setTextColor(MUTED,BG); tft.drawString(matchLine,160,172,2);
  drawButton({14,195,140,36,"EDIT LAST GAME",ORANGE,TEXT},1);
  drawButton({166,195,140,36,"MATCH CARD",LIME,BG},1);
}

void drawResult(){
  saveMatchHistory();reportToHub("complete");lastStandardMatchAvailable=true;drawResultCard();
}

void editLastCompletedGame(){
  if(completedGames<=0)return;
  int game=completedGames-1;
  int winner=gameScore[0][game]>gameScore[1][game]?0:1;
  if(gamesWon[winner]>0)gamesWon[winner]--;
  sideScore[0]=gameScore[leftPlayer][game];sideScore[1]=gameScore[1-leftPlayer][game];
  completedGames--;editingCompletedMatch=true;lastStandardMatchAvailable=false;
  drawScore();reportToHub("playing");
}

void setup(){
  Serial.begin(115200); pinMode(TFT_BL,OUTPUT); digitalWrite(TFT_BL,HIGH);
  tft.init(); tft.setRotation(1); tft.setTextPadding(0);tft.setSwapBytes(true);
  touchSPI.begin(T_CLK,T_MISO,T_MOSI,T_CS); touch.begin(touchSPI); touch.setRotation(1);
  tft.fillScreen(BG);tft.pushImage(120,34,SCOREKEEPER_LOGO_SPLASH_SIZE,SCOREKEEPER_LOGO_SPLASH_SIZE,SCOREKEEPER_LOGO_SPLASH);
  tft.setTextDatum(MC_DATUM);tft.setTextColor(TEXT,BG);tft.drawString("SCOREKEEPER",160,134,4);
  tft.setTextColor(MUTED,BG);tft.drawString("7-6 AIR HOCKEY",160,164,2);delay(900);
  preferences.begin("td-wifi",false);loadSavedNetworks();
  autoWifiEnabled=preferences.getBool("autoWifi",false);
  hubIP=preferences.getString("hubIP","");
  venueTableCount=constrain(preferences.getInt("tableCount",5),1,30);deviceTable=constrain(preferences.getInt("deviceTable",1),1,venueTableCount);
  historyCount=constrain(preferences.getInt("historyCount",0),0,10);
  WiFi.mode(WIFI_STA);drawGamesMenu();startAutoWifi();
}

void loop(){
  serviceAutoWifi();
  serviceHubAssignmentPolling();
  updatePhotonGame();
  if(screen==TIMEOUT_SCREEN){
    updateTimeout();
    if(screen!=TIMEOUT_SCREEN)return;
    if(!touch.tirqTouched()||!touch.touched())return;
    TS_Point timeoutPoint=touch.getPoint();
    uint16_t timeoutX=constrain(map(timeoutPoint.x,TX_MIN,TX_MAX,0,319),0,319);
    uint16_t timeoutY=constrain(map(timeoutPoint.y,TY_MIN,TY_MAX,0,239),0,239);
    if(hit({74,196,172,34,"",0,0},timeoutX,timeoutY)){drawScore();reportToHub("playing");delay(180);}
    return;
  }
  if(!touch.tirqTouched()||!touch.touched())return;
  TS_Point p=touch.getPoint();
  uint16_t x=constrain(map(p.x,TX_MIN,TX_MAX,0,319),0,319), y=constrain(map(p.y,TY_MIN,TY_MAX,0,239),0,239);

  if(screen==SETUP){
    if(hit({137,4,64,27,"",0,0},x,y)){beginTableChange(false);delay(180);return;}
    if(lastStandardMatchAvailable&&((matchCount>0&&hit({4,41,122,25,"",0,0},x,y))||(matchCount==0&&hit({70,181,180,27,"",0,0},x,y)))){drawResultCard();delay(180);return;}
    if(matchCount>0&&hit({218,41,92,25,"",0,0},x,y)){drawManualMatch();delay(180);return;}
    if(matchCount>0&&hit({130,41,84,25,"",0,0},x,y)){selectedHistory=0;drawHistoryList();delay(180);return;}
    int first=matchPage*MATCHES_PER_PAGE,shown=min(MATCHES_PER_PAGE,matchCount-first);
    for(int row=0;row<shown;row++) if(hit({10,73+row*35,300,31,"",0,0},x,y)){
      int i=first+row;if(matches[i].table.toInt()!=deviceTable){delay(180);return;}
      player[0]=matches[i].p1;player[1]=matches[i].p2;selectedTable=matches[i].table.toInt();bestOf=3;isManualMatch=false;
      String format=matches[i].matchType;for(int n=3;n<=9;n+=2)if(format.indexOf(String(n))>=0)bestOf=n;
      leftPlayer=0;sideScore[0]=sideScore[1]=gamesWon[0]=gamesWon[1]=completedGames=0;playerTimeoutUsed[0]=playerTimeoutUsed[1]=false;historySaved=false;editingCompletedMatch=false;lastStandardMatchAvailable=false;memset(gameScore,0,sizeof(gameScore));
      drawScore();reportToHub("playing");delay(180);return;
    }
    int pages=max(1,(matchCount+MATCHES_PER_PAGE-1)/MATCHES_PER_PAGE);
    if(matchCount==0&&hit({10,128,145,45,"",0,0},x,y)){drawManualMatch();}
    else if(matchCount==0&&hit({165,128,145,45,"",0,0},x,y)){selectedHistory=0;drawHistoryList();}
    else if(hit({4,216,76,20,"",0,0},x,y)){scanWifi();drawWifiList();}
    else if(hit({84,216,50,20,"",0,0},x,y)&&matchPage>0){matchPage--;drawSetup();}
    else if(hit({138,216,50,20,"",0,0},x,y)&&matchPage+1<pages){matchPage++;drawSetup();}
    else if(hit({192,216,124,20,"",0,0},x,y)){fetchMatches();drawSetup();}
  }else if(screen==MANUAL_TABLES){
    if(hit({35,92,55,40,"",0,0},x,y)&&venueTableCount>1){venueTableCount--;if(deviceTable>venueTableCount)deviceTable=venueTableCount;drawManualTableSetup();}
    else if(hit({230,92,55,40,"",0,0},x,y)&&venueTableCount<30){venueTableCount++;drawManualTableSetup();}
    else if(hit({35,162,55,40,"",0,0},x,y)&&deviceTable>1){deviceTable--;drawManualTableSetup();}
    else if(hit({230,162,55,40,"",0,0},x,y)&&deviceTable<venueTableCount){deviceTable++;drawManualTableSetup();}
    else if(hit({74,211,172,24,"",0,0},x,y)){preferences.putInt("tableCount",venueTableCount);preferences.putInt("deviceTable",deviceTable);drawManualMatch();}
  }else if(screen==MANUAL_MATCH){
    if(hit({126,4,76,27,"",0,0},x,y)){beginTableChange(true);delay(180);return;}
    if(hit({14,111,91,54,"",0,0},x,y))startManualMatch(3);
    else if(hit({114,111,91,54,"",0,0},x,y))startManualMatch(5);
    else if(hit({214,111,91,54,"",0,0},x,y))startManualMatch(7);
    else if(hit({8,193,96,32,"",0,0},x,y)){scanWifi();drawWifiList();}
    else if(hit({112,193,96,32,"",0,0},x,y))drawGamesMenu();
    else if(hit({216,193,96,32,"",0,0},x,y))openMatchCard();
  }else if(screen==TABLE_PIN){
    const char* values[]={"1","2","3","4","5","6","7","8","9","","0",""};
    for(int i=0;i<12;i++){
      int col=i%3,row=i/3;if(!hit({31+col*87,91+row*31,82,27,"",0,0},x,y))continue;
      tablePinMessage="";
      if(i==9){if(tablePin.length())tablePin.remove(tablePin.length()-1);}
      else if(i==11){if(tableChangeFromManual)drawManualMatch();else drawSetup();delay(180);return;}
      else if(tablePin.length()<4)tablePin+=values[i];
      if(tablePin.length()==4){
        if(tablePin=="2580"){drawTableSelect();delay(180);return;}
        tablePin="";tablePinMessage="WRONG CODE - TRY AGAIN";
      }
      drawTablePin();delay(120);return;
    }
  }else if(screen==TABLE_SELECT){
    for(int table=1;table<=30;table++){
      int index=table-1,col=index%6,row=index/6;
      if(hit({4+col*53,60+row*29,49,25,"",0,0},x,y)){saveDefaultTable(table);delay(180);return;}
    }
    if(hit({110,211,100,24,"",0,0},x,y)){if(tableChangeFromManual)drawManualMatch();else drawSetup();}
  }else if(screen==MANUAL_NAMES){
    const int lengths[]={10,9,manualKeyboardMode==2?8:7};const int starts[]={4,19,manualKeyboardMode==2?35:50};
    for(int row=0;row<3;row++)for(int col=0;col<lengths[row];col++){
      if(!hit({starts[row]+col*31,79+row*32,28,28,"",0,0},x,y))continue;
      if(!manualNameEdited[editingPlayer]){player[editingPlayer]="";manualNameEdited[editingPlayer]=true;}
      if(player[editingPlayer].length()<16)player[editingPlayer]+=manualKey(row,col);
      drawManualNames();delay(120);return;
    }
    if(hit({4,177,48,27,"",0,0},x,y)){
      manualKeyboardMode=manualKeyboardMode==0?1:0;drawManualNames();
    }else if(hit({56,177,48,27,"",0,0},x,y)){
      manualKeyboardMode=manualKeyboardMode==2?0:2;drawManualNames();
    }else if(hit({108,177,74,27,"",0,0},x,y)){
      if(!manualNameEdited[editingPlayer]){player[editingPlayer]="";manualNameEdited[editingPlayer]=true;}
      if(player[editingPlayer].length()&&player[editingPlayer].length()<16)player[editingPlayer]+=" ";drawManualNames();
    }else if(hit({186,177,48,27,"",0,0},x,y)){
      if(!manualNameEdited[editingPlayer]){player[editingPlayer]="";manualNameEdited[editingPlayer]=true;}
      else if(player[editingPlayer].length())player[editingPlayer].remove(player[editingPlayer].length()-1);
      drawManualNames();
    }else if(hit({238,177,78,27,"",0,0},x,y)){
      player[editingPlayer]=editingPlayer==0?"Player 1":"Player 2";manualNameEdited[editingPlayer]=false;drawManualNames();
    }else if(hit({82,211,234,24,"",0,0},x,y)){
      if(!player[editingPlayer].length())player[editingPlayer]=editingPlayer==0?"Player 1":"Player 2";
      if(editingPlayer==0){editingPlayer=1;manualKeyboardMode=0;drawManualNames();}else beginManualScoring();
    }else if(hit({4,211,74,24,"",0,0},x,y)){
      if(editingPlayer==1){editingPlayer=0;drawManualNames();}else drawManualMatch();
    }
  }else if(screen==HISTORY_LIST){
    for(int row=0;row<historyVisibleCount;row++)if(hit({10,72+row*34,300,30,"",0,0},x,y)){selectedHistory=historyVisibleIndex[row];drawHistoryList();delay(180);return;}
    if(hit({202,39,108,23,"",0,0},x,y)){historyFilter=(historyFilter+1)%3;historyPage=0;drawHistoryList();delay(150);return;}
    if(hit({3,218,55,19,"",0,0},x,y))drawSetup();
    else if(hit({62,218,54,19,"",0,0},x,y)&&historyPage>0){historyPage--;drawHistoryList();}
    else if(hit({120,218,54,19,"",0,0},x,y)&&historyPage+1<historyPageCount){historyPage++;drawHistoryList();}
    else if(historyVisibleCount&&hit({178,218,139,19,"",0,0},x,y))drawHistoryDetail();
  }else if(screen==HISTORY_DETAIL){
    if(hit({74,207,172,27,"",0,0},x,y))drawHistoryList();
  }else if(screen==WIFI_LIST){
    int first=wifiPage*WIFI_NETWORKS_PER_PAGE;
    int shown=min(WIFI_NETWORKS_PER_PAGE,networkCount-first);
    for(int row=0;row<shown;row++){
      int i=first+row;
      if(networkSaved[i]&&hit({234,65+row*34,76,30,"",0,0},x,y)){
        if(autoWifiTrying)WiFi.disconnect(false,false);autoWifiTrying=false;forgetNetwork(savedWifiIndex(networkName[i]));
        scanWifi();if(autoWifiEnabled)startAutoWifi();drawWifiList();delay(180);return;
      }
      if(!hit({10,65+row*34,networkSaved[i]?220:300,30,"",0,0},x,y))continue;
      pendingSSID=networkName[i];pendingPassword="";keyboardMode=0;
      int savedIndex=savedWifiIndex(pendingSSID);
      if(savedIndex>=0){pendingPassword=savedPassword[savedIndex];connectPendingWifi();}
      else if(networkOpen[i])connectPendingWifi();else drawWifiPassword();
      delay(180);return;
    }
    int pageCount=max(1,(networkCount+WIFI_NETWORKS_PER_PAGE-1)/WIFI_NETWORKS_PER_PAGE);
    if(hit({190,39,120,23,"",0,0},x,y)){
      autoWifiEnabled=!autoWifiEnabled;preferences.putBool("autoWifi",autoWifiEnabled);
      if(autoWifiEnabled)startAutoWifi();else stopAutoWifi();drawWifiList();delay(180);return;
    }
    if(hit({3,218,55,19,"",0,0},x,y))drawGamesMenu();
    else if(hit({62,218,54,19,"",0,0},x,y)&&wifiPage>0){wifiPage--;drawWifiList();}
    else if(hit({120,218,54,19,"",0,0},x,y)&&wifiPage+1<pageCount){wifiPage++;drawWifiList();}
    else if(hit({178,218,75,19,"",0,0},x,y)){hubTestMessage="";drawHubSetup();}
    else if(hit({257,218,60,19,"",0,0},x,y)){scanWifi();drawWifiList();}
  }else if(screen==WIFI_PASSWORD){
    const int lengths[]={10,9,keyboardMode==2?8:7};const int starts[]={4,19,keyboardMode==2?35:50};
    for(int row=0;row<3;row++)for(int col=0;col<lengths[row];col++){
      if(!hit({starts[row]+col*31,78+row*31,28,27,"",0,0},x,y))continue;
      if(pendingPassword.length()<63)pendingPassword+=wifiKey(row,col);drawWifiPassword();delay(100);return;
    }
    if(hit({4,173,54,26,"",0,0},x,y)){keyboardMode=keyboardMode==0?1:0;drawWifiPassword();}
    else if(hit({62,173,54,26,"",0,0},x,y)){keyboardMode=keyboardMode==2?0:2;drawWifiPassword();}
    else if(hit({120,173,104,26,"",0,0},x,y)){if(pendingPassword.length()<63)pendingPassword+=" ";drawWifiPassword();}
    else if(hit({228,173,88,26,"",0,0},x,y)){if(pendingPassword.length())pendingPassword.remove(pendingPassword.length()-1);drawWifiPassword();}
    else if(hit({4,207,104,27,"",0,0},x,y)){drawWifiList();delay(180);return;}
    else if(hit({112,207,204,27,"",0,0},x,y)){connectPendingWifi();delay(180);return;}
  }else if(screen==HUB_SETUP){
    const char* keyValues[]={"1","2","3","4","5","6","7","8","9",".","0",""};
    for(int i=0;i<12;i++){
      int col=i%3,row=i/3;if(!hit({8+col*57,88+row*34,52,30,"",0,0},x,y))continue;
      if(i==11&&hubIP.length())hubIP.remove(hubIP.length()-1);else if(i<11&&hubIP.length()<15)hubIP+=keyValues[i];hubConnected=false;hubTestMessage="";drawHubSetup();delay(150);return;
    }
    if(hit({190,88,122,38,"",0,0},x,y)){testHubConnection();delay(180);return;}
    else if(hit({190,134,122,38,"",0,0},x,y)){
      IPAddress parsed;if(parsed.fromString(hubIP)){preferences.putString("hubIP",hubIP);hubTestMessage="SAVED";}else hubTestMessage="INVALID IP ADDRESS";drawHubSetup();
    }
    else if(hit({190,180,122,38,"",0,0},x,y))drawWifiList();
  }else if(screen==GAMES_MENU){
    if(hit({10,70,145,104,"",0,0},x,y))drawManualMatch();
    else if(hit({165,70,145,104,"",0,0},x,y))beginPhotonNames();
    else if(hit({10,194,145,34,"",0,0},x,y)){scanWifi();drawWifiList();}
    else if(hit({165,194,145,34,"",0,0},x,y))openMatchCard();
  }else if(screen==PHOTON_NAMES){
    const int lengths[]={10,9,photonKeyboardMode==2?8:7};const int starts[]={4,19,photonKeyboardMode==2?35:50};
    for(int row=0;row<3;row++)for(int col=0;col<lengths[row];col++){
      if(!hit({starts[row]+col*31,79+row*32,28,28,"",0,0},x,y))continue;
      if(!photonNameEdited[photonEditingTeam]){photonName[photonEditingTeam]="";photonNameEdited[photonEditingTeam]=true;}
      if(photonName[photonEditingTeam].length()<16)photonName[photonEditingTeam]+=photonKey(row,col);drawPhotonNames();delay(100);return;
    }
    if(hit({4,177,48,27,"",0,0},x,y)){
      photonKeyboardMode=photonKeyboardMode==0?1:0;drawPhotonNames();
    }else if(hit({56,177,48,27,"",0,0},x,y)){
      photonKeyboardMode=photonKeyboardMode==2?0:2;drawPhotonNames();
    }else if(hit({108,177,74,27,"",0,0},x,y)){
      if(!photonNameEdited[photonEditingTeam]){photonName[photonEditingTeam]="";photonNameEdited[photonEditingTeam]=true;}
      if(photonName[photonEditingTeam].length()&&photonName[photonEditingTeam].length()<16)photonName[photonEditingTeam]+=" ";drawPhotonNames();
    }else if(hit({186,177,48,27,"",0,0},x,y)){
      if(!photonNameEdited[photonEditingTeam]){photonName[photonEditingTeam]="";photonNameEdited[photonEditingTeam]=true;}
      else if(photonName[photonEditingTeam].length())photonName[photonEditingTeam].remove(photonName[photonEditingTeam].length()-1);drawPhotonNames();
    }else if(hit({238,177,78,27,"",0,0},x,y)){
      photonName[photonEditingTeam]=photonEditingTeam==0?"RED TEAM":"BLUE TEAM";photonNameEdited[photonEditingTeam]=false;drawPhotonNames();
    }else if(hit({82,211,234,24,"",0,0},x,y)){
      if(!photonName[photonEditingTeam].length())photonName[photonEditingTeam]=photonEditingTeam==0?"RED TEAM":"BLUE TEAM";
      if(photonEditingTeam==0){photonEditingTeam=1;photonKeyboardMode=0;drawPhotonNames();}else startPhotonMatch();
    }else if(hit({4,211,74,24,"",0,0},x,y)){
      if(photonEditingTeam==1){photonEditingTeam=0;drawPhotonNames();}else drawGamesMenu();
    }
  }else if(screen==PHOTON_GAME){
    int leftTeam=photonSidesSwapped?1:0,rightTeam=1-leftTeam;
    if(hit({0,0,44,36,"",0,0},x,y)){
      photonRunning=false;photonTimeoutActive=false;reportPhotonToHub("abandoned");drawGamesMenu();delay(180);return;
    }
    if(hit({145,5,30,26,"",0,0},x,y)){photonSidesSwapped=!photonSidesSwapped;drawPhotonGame();reportPhotonToHub("playing");delay(150);return;}
    if(hit({272,39,44,34,"",0,0},x,y)){
      photonTimeoutTeam=-1;photonRunning=false;photonTimeoutActive=true;photonTimeoutEnds=millis()+10000;drawPhotonGame();reportPhotonToHub("timeout");delay(120);return;
    }
    if(hit({4,79,152,91,"",0,0},x,y)){addPhotonGoal(leftTeam);delay(150);return;}
    if(hit({164,79,152,91,"",0,0},x,y)){addPhotonGoal(rightTeam);delay(150);return;}
    if(hit({4,173,104,27,"",0,0},x,y)){removePhotonGoal(leftTeam);delay(150);return;}
    if(hit({212,173,104,27,"",0,0},x,y)){removePhotonGoal(rightTeam);delay(150);return;}
    if(hit({4,204,104,32,"",0,0},x,y)&&!photonTimeoutUsed[leftTeam]){
      photonTimeoutUsed[leftTeam]=true;photonTimeoutTeam=leftTeam;photonRunning=false;photonTimeoutActive=true;photonTimeoutEnds=millis()+10000;drawPhotonGame();reportPhotonToHub("timeout");delay(120);return;
    }
    if(hit({212,204,104,32,"",0,0},x,y)&&!photonTimeoutUsed[rightTeam]){
      photonTimeoutUsed[rightTeam]=true;photonTimeoutTeam=rightTeam;photonRunning=false;photonTimeoutActive=true;photonTimeoutEnds=millis()+10000;drawPhotonGame();reportPhotonToHub("timeout");delay(120);return;
    }
    if(hit({112,173,96,63,"",0,0},x,y)){
      if(photonTimeoutActive)photonTimeoutActive=false;
      photonRunning=!photonRunning;if(photonRunning){photonLastTick=millis();reportPhotonToHub("playing");}else reportPhotonToHub("paused");drawPhotonGame();delay(180);return;
    }
  }else if(screen==PHOTON_RESULT){
    if(hit({14,194,142,34,"",0,0},x,y))drawGamesMenu();
    else if(hit({164,194,142,34,"",0,0},x,y)){
      if(photonMatchComplete)beginPhotonNames();
      else{photonSidesSwapped=!photonSidesSwapped;startPhotonGame();}
    }
  }else if(screen==INITIALS){
    for(int i=0;i<28;i++){
      int col=i%7,row=i/7; if(!hit({8+col*44,91+row*35,40,31,"",0,0},x,y))continue;
      if(i<26&&player[editingPlayer].length()<3)player[editingPlayer]+=(char)('A'+i);
      else if(i==26&&player[editingPlayer].length()>0)player[editingPlayer].remove(player[editingPlayer].length()-1);
      else if(i==27&&player[editingPlayer].length()>0){if(editingPlayer==0){editingPlayer=1;drawInitials();delay(180);return;}leftPlayer=0;sideScore[0]=sideScore[1]=gamesWon[0]=gamesWon[1]=completedGames=0;memset(gameScore,0,sizeof(gameScore));drawScore();delay(180);return;}
      drawInitials();break;
    }
  }else if(screen==SCORE){
    if(hit({0,0,44,36,"",0,0},x,y)){
      reportToHub("abandoned");fetchMatches();drawSetup();delay(180);return;
    }
    else if(hit({145,5,30,26,"",0,0},x,y)){
      leftPlayer=1-leftPlayer;int heldScore=sideScore[0];sideScore[0]=sideScore[1];sideScore[1]=heldScore;drawScore();reportToHub("playing");delay(180);return;
    }
    else if(hit({10,53,143,94,"",0,0},x,y))sideScore[0]++;
    else if(hit({167,53,143,94,"",0,0},x,y))sideScore[1]++;
    else if(hit({62,153,39,27,"",0,0},x,y)&&sideScore[0]>0)sideScore[0]--;
    else if(hit({219,153,39,27,"",0,0},x,y)&&sideScore[1]>0)sideScore[1]--;
    else if(hit({5,188,76,47,"",0,0},x,y)){
      int id=sidePlayer(0);if(playerTimeoutUsed[id])return;playerTimeoutUsed[id]=true;startTimeout(fitName(player[id],8)+" TIME");delay(180);return;
    }
    else if(hit({88,188,144,47,"",0,0},x,y)){startTimeout("REF TIME OUT");delay(180);return;}
    else if(hit({239,188,76,47,"",0,0},x,y)){
      int id=sidePlayer(1);if(playerTimeoutUsed[id])return;playerTimeoutUsed[id]=true;startTimeout(fitName(player[id],8)+" TIME");delay(180);return;
    }
    else return;
    if(sideScore[0]>=7||sideScore[1]>=7){drawGameConfirm();reportToHub("confirm_score");}else{drawScore();reportToHub("playing");}
  }else if(screen==CONFIRM_GAME){
    if(hit({14,151,140,62,"",0,0},x,y))drawScore();
    else if(hit({166,151,140,62,"",0,0},x,y))saveGame();
  }else if(screen==SWITCH_SIDES&&hit({44,174,232,46,"",0,0},x,y)){
    leftPlayer=1-leftPlayer;sideScore[0]=sideScore[1]=0;playerTimeoutUsed[0]=playerTimeoutUsed[1]=false;drawScore();reportToHub("playing");
  }else if(screen==RESULT){
    if(hit({14,195,140,36,"",0,0},x,y)){editLastCompletedGame();delay(180);return;}
    else if(hit({166,195,140,36,"",0,0},x,y)){openMatchCard();delay(180);return;}
  }
  delay(180);
}
