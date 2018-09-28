#include "InputManager.h"
#include "InputConfig.h"
//#include "Settings.h"
//#include "Window.h"
//#include "Log.h"
#include "pugixml/pugixml.hpp"
#include <boost/filesystem.hpp>
//#include "platform.h"
#include <iostream>

#include <stdio.h>
#include <execinfo.h>
#include <signal.h>
#include <stdlib.h>
#include <unistd.h>

#include "MenuScreen.h"
extern "C"{
#include "yabause.h"
#include "peripheral.h"
}

using namespace::std;

#include <iostream>
#include <fstream>
#include <sstream>
#include <iomanip>

#include <nlohmann/json.hpp>
using json = nlohmann::json;

#define PADLOG 
//#define PADLOG printf

#define KEYBOARD_MASK 0x80000000
#define SDL_MAX_AXIS_VALUE 0x110000
#define SDL_MIN_AXIS_VALUE 0x100000
#define SDL_HAT_VALUE 0x200000
#define SDL_MEDIUM_AXIS_VALUE (int)(32768 / 2)
#define SDL_BUTTON_PRESSED 1
#define SDL_BUTTON_RELEASED 0
//unsigned int SDL_PERCORE_INITIALIZED = 0;
//unsigned int SDL_PERCORE_JOYSTICKS_INITIALIZED = 0;
static unsigned int SDL_HAT_VALUES[] = { SDL_HAT_UP, SDL_HAT_RIGHT, SDL_HAT_LEFT, SDL_HAT_DOWN };
static const unsigned int SDL_HAT_VALUES_NUM = sizeof(SDL_HAT_VALUES) / sizeof(SDL_HAT_VALUES[0]);

#define KEYBOARD_GUID_STRING "-1"

// SO HEY POTENTIAL POOR SAP WHO IS TRYING TO MAKE SENSE OF ALL THIS (by which I mean my future self)
// There are like four distinct IDs used for joysticks (crazy, right?)
// 1. Device index - this is the "lowest level" identifier, and is just the Nth joystick plugged in to the system (like /dev/js#).
//    It can change even if the device is the same, and is only used to open joysticks (required to receive SDL events).
// 2. SDL_JoystickID - this is an ID for each joystick that is supposed to remain consistent between plugging and unplugging.
//    ES doesn't care if it does, though.
// 3. "Device ID" - this is something I made up and is what InputConfig's getDeviceID() returns.  
//    This is actually just an SDL_JoystickID (also called instance ID), but -1 means "keyboard" instead of "error."
// 4. Joystick GUID - this is some squashed version of joystick vendor, version, and a bunch of other device-specific things.
//    It should remain the same across runs of the program/system restarts/device reordering and is what I use to identify which joystick to load.

namespace fs = boost::filesystem;

InputManager* InputManager::mInstance = NULL;

InputManager::InputManager() : mKeyboardInputConfig(NULL)
{
}

InputManager::~InputManager()
{
  deinit();
}

InputManager* InputManager::getInstance()
{
  if(!mInstance)
    mInstance = new InputManager();

  return mInstance;
}

#define MAKE_PAD(a,b) ((a<<24)|(b))

uint32_t genid( int user, int joyId, const Input & result ){
  if( result.type == TYPE_AXIS ){
    return MAKE_PAD(0,((joyId << 18)|SDL_MAX_AXIS_VALUE|result.id));
  }else{
    return MAKE_PAD(0,((joyId << 18)|result.id));
  }
}


uint32_t genidjson( int user, int joyId, const json & result ){

  PADLOG("Keymap: user %d, joyid %d, type %s, id %d\n", 0, joyId, result["type"].get<string>().c_str(), result["id"].get<int>());

  if( result["type"] == "axis" ){
    return MAKE_PAD(0,((joyId << 18)|SDL_MEDIUM_AXIS_VALUE|result["id"].get<int>() ));
  }else if( result["type"] == "hat" ){
    return MAKE_PAD(0, (joyId << 18) | SDL_HAT_VALUE | ( (result["value"].get<int>()) <<4));
  }else if( result["type"] == "key" ){
    return result["id"].get<int>() | KEYBOARD_MASK ;
  }
  return MAKE_PAD(0,((joyId << 18)|result["id"].get<int>()));
}


int setPlayerKeys( void * padbits, int user, int joyId, const json & player ){
    PerSetKey(genidjson(user,joyId,player["up"]),PERPAD_UP, padbits);
    PerSetKey(genidjson(user,joyId,player["down"]),PERPAD_DOWN, padbits);
    PerSetKey(genidjson(user,joyId,player["left"]),PERPAD_LEFT, padbits);
    PerSetKey(genidjson(user,joyId,player["right"]),PERPAD_RIGHT, padbits);
    PerSetKey(genidjson(user,joyId,player["start"]), PERPAD_START, padbits);
    PerSetKey(genidjson(user,joyId,player["a"]),PERPAD_A, padbits);
    PerSetKey(genidjson(user,joyId,player["b"]),PERPAD_B, padbits);
    PerSetKey(genidjson(user,joyId,player["c"]),PERPAD_C, padbits);
    PerSetKey(genidjson(user,joyId,player["x"]),PERPAD_X, padbits);
    PerSetKey(genidjson(user,joyId,player["y"]),PERPAD_Y, padbits);
    PerSetKey(genidjson(user,joyId,player["z"]),PERPAD_Z, padbits);
    PerSetKey(genidjson(user,joyId,player["l"]),PERPAD_LEFT_TRIGGER, padbits);
    PerSetKey(genidjson(user,joyId,player["r"]),PERPAD_RIGHT_TRIGGER, padbits);    
    PerSetKey(genidjson(user,joyId,player["analogx"]), PERANALOG_AXIS1, padbits);
    PerSetKey(genidjson(user,joyId,player["analogy"]), PERANALOG_AXIS2, padbits);
    PerSetKey(genidjson(user,joyId,player["analogleft"]), PERANALOG_AXIS3, padbits);
    PerSetKey(genidjson(user,joyId,player["analogright"]), PERANALOG_AXIS4, padbits);  
}

int setDefalutSettings( void * padbits ){

    // Set Defaults
    PerSetKey(SDLK_UP,PERPAD_UP, padbits);
    PerSetKey(SDLK_DOWN,PERPAD_DOWN, padbits);
    PerSetKey(SDLK_LEFT,PERPAD_LEFT, padbits);
    PerSetKey(SDLK_RIGHT,PERPAD_RIGHT, padbits);
    PerSetKey(SDLK_RETURN, PERPAD_START, padbits);
    PerSetKey(SDLK_z,PERPAD_A, padbits);
    PerSetKey(SDLK_x,PERPAD_B, padbits);
    PerSetKey(SDLK_c,PERPAD_C, padbits);
    PerSetKey(SDLK_a,PERPAD_X, padbits);
    PerSetKey(SDLK_s,PERPAD_Y, padbits);
    PerSetKey(SDLK_d,PERPAD_Z, padbits);
    PerSetKey(SDLK_q,PERPAD_LEFT_TRIGGER, padbits);
    PerSetKey(SDLK_e,PERPAD_RIGHT_TRIGGER, padbits); 

    return 0;
}

int mapKeys( const json & configs ){
  void * padbits;
  int user = 0;
  PerPortReset();

  if( SDL_NumJoysticks() == 0 ){

    PADLOG("No joy stic is found force to keyboard\n");
    padbits = PerPadAdd(&PORTDATA1);
    if( configs.find("player1") == configs.end() ){
      return setDefalutSettings(padbits);
    }
    json p = configs["player1"];
    if( p["DeviceID"].get<int>() == -1 ){
      std::string guid = p["deviceGUID"];
      json dev = configs[ guid ];
      setPlayerKeys( padbits, 0, -1, dev );
      return 0;
    }
    if( configs.find("player2") == configs.end() ){
      return setDefalutSettings(padbits);
    }    
    p = configs["player2"];
    if( p["DeviceID"].get<int>() == -1 ){
      std::string guid = p["deviceGUID"];
      json dev = configs[ guid ];
      setPlayerKeys( padbits, 0, -1, p );
      return 0;
    }    
    return setDefalutSettings(padbits);
  }

  if( configs.find("player1") == configs.end() ) return setDefalutSettings(padbits);

  json p1 = configs["player1"];

  try {
    if( !p1.is_null() ){
      string guid = p1["deviceGUID"];
      int joyId = -2;
      if( p1["DeviceID"] == -1 ){
        PADLOG("Player1: %s is selected\n",p1["deviceName"].get<std::string>().c_str() );
        joyId = -1;
      }else{
        for( int i=0; i<SDL_NumJoysticks(); i++ ){
          char cguid[65];
          SDL_Joystick* joy = SDL_JoystickOpen(i);
          if( joy != NULL ){
            SDL_JoystickGetGUIDString(SDL_JoystickGetGUID(joy), cguid, 65);
            if( guid == string(cguid) ){
              SDL_JoystickID _joyId = SDL_JoystickInstanceID(joy);
              PADLOG("Player1: %s(%d) is selected\n",p1["deviceName"].get<std::string>().c_str(), _joyId );            
              joyId = _joyId;
            }
            if (SDL_JoystickGetAttached(joy)) {
              SDL_JoystickClose(joy);
            }        
          }
        }
      }
      if( joyId != -2 ) {
        if( p1["padmode"].is_null() || p1["padmode"] == 0 ){
          padbits = PerPadAdd(&PORTDATA1);
          PADLOG("Player1: Switch to Pad mode\n");
        }else{
          padbits = Per3DPadAdd(&PORTDATA1);
          PADLOG("Player1: Switch to Analog mode\n");
        }
        string guid = p1["deviceGUID"];
        json dev = configs[ guid ];
        setPlayerKeys( padbits, user, joyId, dev );
      }
    }
  }catch( json::type_error & e ){
         std::cout << "message: " << e.what() << '\n'
                   << "exception id: " << e.id << std::endl;
/*
        void *array[10];
        size_t size;
        size = backtrace(array, 10);
        backtrace_symbols_fd(array, size, STDERR_FILENO);
*/
        void *trace_elems[20];
        int trace_elem_count(backtrace(trace_elems, 20));
        char **stack_syms(backtrace_symbols(trace_elems, trace_elem_count));
        for(int i=0 ; i != trace_elem_count; ++i)
        {
            cout << stack_syms[i] << "\n";
        }
        free(stack_syms);

        return -1;
  }

  // Is there player2 settings?
  if( configs.find("player2") == configs.end() ) return 0;

  json p2;
  p2 = configs["player2"];

  try {
    user = 1;
    if( !p2.is_null() ){
      string guid = p2["deviceGUID"];
      int joyId = -2;
      if( p2["DeviceID"] == -1 ){
        PADLOG("Player2: %s is selected\n",p2["deviceName"].get<std::string>().c_str() );
        joyId = -1;
      }else{
        for( int i=0; i<SDL_NumJoysticks(); i++ ){
          char cguid[65];
          SDL_Joystick* joy = SDL_JoystickOpen(i);
          if( joy != NULL ){
            SDL_JoystickGetGUIDString(SDL_JoystickGetGUID(joy), cguid, 65);
            if( guid == string(cguid) ){
              SDL_JoystickID _joyId = SDL_JoystickInstanceID(joy);
              PADLOG("Player2: %s(%d) is selected\n",p2["deviceName"].get<std::string>().c_str(), _joyId );
              joyId = _joyId;
            }
            if (SDL_JoystickGetAttached(joy)) {
              SDL_JoystickClose(joy);
            }         
          }
        }
      }
      if( joyId != -2 ) {
        if( p2["padmode"].is_null() || p2["padmode"] == 0 ){
          padbits = PerPadAdd(&PORTDATA2);
          PADLOG("Player2: Switch to Pad mode\n");
        }else{
          padbits = Per3DPadAdd(&PORTDATA2);
          PADLOG("Player2: Switch to Analog mode\n");
        }
        string guid = p2["deviceGUID"];
        json dev = configs[ guid ];
        setPlayerKeys( padbits, user, joyId, dev );
      }
    }
  }catch( json::type_error & e){
         std::cout << "message: " << e.what() << '\n'
                   << "exception id: " << e.id << std::endl;
        return -1;
  }
  return 0;
}

void InputManager::updateConfig(){
  json j;
  std::ifstream fin( config_fname_ );
  fin >> j;
  fin.close();
  mapKeys(j);
}

int InputManager::getCurrentPadMode( int user ){
  json j;
  std::ifstream fin( config_fname_ );
  fin >> j;
  fin.close();
  if( user == 0 ){
    return j["player1"]["padmode"];
  }
  return 0;
}

void InputManager::setGamePadomode( int user, int mode ){

  json j;
  std::ifstream fin( config_fname_ );
  fin >> j;
  fin.close();

  if( user == 0 ){
    PADLOG("User mode %d\n", mode );
    j["player1"]["padmode"] = mode;
    mapKeys(j);
    std::ofstream out(config_fname_);
    out << j.dump(2);
    out.close();
  }

#if 0  
  void * padbits;
  Input result;  
  int numJoysticks = SDL_NumJoysticks();
  
  if( numJoysticks != 0 ){
    
    int joyId = mInputConfigs[0]->getDeviceId();

    if( mode == 0 ){
      LOG("Switch to dig mode\n");
      PerPortReset();
      if( user == 0 ){
        padbits = PerPadAdd(&PORTDATA1);    
      }else{
        padbits = PerPadAdd(&PORTDATA2);    
      }
    }else if( mode == 1 ){
      LOG("Switch to Analog mode\n");
            PerPortReset();
      if( user == 0 ){
              padbits = Per3DPadAdd(&PORTDATA1);
      }else{
        padbits = Per3DPadAdd(&PORTDATA2);
      }
      PerSetKey(MAKE_PAD(user,((joyId << 18)|SDL_MEDIUM_AXIS_VALUE|0)), PERANALOG_AXIS1, padbits);
      PerSetKey(MAKE_PAD(user,((joyId << 18)|SDL_MEDIUM_AXIS_VALUE|1)), PERANALOG_AXIS2, padbits);
      PerSetKey(MAKE_PAD(user,((joyId << 18)|SDL_MEDIUM_AXIS_VALUE|2)), PERANALOG_AXIS3, padbits);
      PerSetKey(MAKE_PAD(user,((joyId << 18)|SDL_MEDIUM_AXIS_VALUE|3)), PERANALOG_AXIS4, padbits);
    }

    mInputConfigs[0]->getInputByName("up", &result);
    PerSetKey(genid(user,joyId,result),PERPAD_UP, padbits);
    mInputConfigs[0]->getInputByName("down", &result);
    PerSetKey(genid(user,joyId,result), PERPAD_DOWN, padbits);
    mInputConfigs[0]->getInputByName("left", &result);
    PerSetKey(genid(user,joyId,result), PERPAD_LEFT, padbits);
    mInputConfigs[0]->getInputByName("right", &result);
    PerSetKey(genid(user,joyId,result), PERPAD_RIGHT, padbits);
    mInputConfigs[0]->getInputByName("start", &result);
    PerSetKey(genid(user,joyId,result), PERPAD_START, padbits);
    mInputConfigs[0]->getInputByName("select", &result);
    select_button_ = result.id;    
    mInputConfigs[0]->getInputByName("a", &result);
    PerSetKey(genid(user,joyId,result), PERPAD_A, padbits);
    mInputConfigs[0]->getInputByName("b", &result);
    PerSetKey(genid(user,joyId,result), PERPAD_B, padbits);
    mInputConfigs[0]->getInputByName("x", &result);
    PerSetKey(genid(user,joyId,result), PERPAD_X, padbits);
    mInputConfigs[0]->getInputByName("y", &result);
    PerSetKey(genid(user,joyId,result) , PERPAD_Y, padbits);
    mInputConfigs[0]->getInputByName("rightshoulder", &result);
    PerSetKey(genid(user,joyId,result), PERPAD_C, padbits);
    mInputConfigs[0]->getInputByName("leftshoulder", &result);
    PerSetKey(genid(user,joyId,result), PERPAD_Z, padbits);

    mInputConfigs[0]->getInputByName("lefttrigger", &result);
    PerSetKey(genid(user,joyId,result), PERPAD_LEFT_TRIGGER, padbits);

    mInputConfigs[0]->getInputByName("righttrigger", &result);
    PerSetKey(genid(user,joyId,result), PERPAD_RIGHT_TRIGGER, padbits);
    
      
    // Force Hat
    PerSetKey( MAKE_PAD(user, (joyId << 18) | SDL_HAT_VALUE | (SDL_HAT_UP<<4)), PERPAD_UP, padbits);
    PerSetKey( MAKE_PAD(user,(joyId << 18) | SDL_HAT_VALUE | (SDL_HAT_DOWN<<4)), PERPAD_DOWN, padbits);
    PerSetKey( MAKE_PAD(user,(joyId << 18) | SDL_HAT_VALUE | (SDL_HAT_LEFT<<4)), PERPAD_LEFT, padbits);
    PerSetKey( MAKE_PAD(user,(joyId << 18) | SDL_HAT_VALUE | (SDL_HAT_RIGHT<<4)), PERPAD_RIGHT, padbits);
  }
#endif  
}

/*
struct Input
{
public:
  int device;
  InputType type;
  int id;
  int value;
  bool configured;
*/

const std::string input_types[] = {
  "axis",
  "button",
  "hat",
  "key"
};


int getPlayerJsonFromInputConfig( int joy, InputConfig * inputconfig, json & player ) {
  
  if( inputconfig == NULL ){
    return -1;
  }

  char p[64];
  sprintf(p,"player%d",joy+1);

  std::string guid;

  player[p]["padmode"] = 0;
  player[p]["DeviceID"]   = inputconfig->getDeviceId();
  player[p]["deviceName"] = inputconfig->getDeviceName();
  player[p]["deviceGUID"] = inputconfig->getDeviceGUIDString();
  guid = inputconfig->getDeviceGUIDString();

  // Keyborad
  if( inputconfig->getDeviceId() == -1 ){ 
    player[p]["padmode"] = 0;
  }

  Input result;
  inputconfig->getInputByName("a", &result);
  player[guid]["a"] ={ { "type", input_types[result.type] },{ "id", result.id },{ "value", result.value } };

  inputconfig->getInputByName("b", &result);
  player[guid]["b"] = { { "type", input_types[result.type] },{ "id", result.id },{ "value", result.value } };

  inputconfig->getInputByName("rightshoulder", &result);
  player[guid]["c"] = { { "type", input_types[result.type] },{ "id", result.id },{ "value", result.value } } ;

  inputconfig->getInputByName("x", &result);
  player[guid]["x"] = { { "type", input_types[result.type] },{ "id", result.id },{ "value", result.value } };

  inputconfig->getInputByName("y", &result);
  player[guid]["y"] = { { "type", input_types[result.type] },{ "id", result.id },{ "value", result.value } };

  inputconfig->getInputByName("leftshoulder", &result);
  player[guid]["z"] = { { "type", input_types[result.type] },{ "id", result.id },{ "value", result.value } };

  inputconfig->getInputByName("lefttrigger", &result);
  player[guid]["l"] = { { "type", input_types[result.type] },{ "id", result.id },{ "value", result.value } } ;

  inputconfig->getInputByName("righttrigger", &result);
  player[guid]["r"] = { { "type", input_types[result.type] },{ "id", result.id },{ "value", result.value } };

  inputconfig->getInputByName("up", &result);
  player[guid]["up"] = { { "type", input_types[result.type] },{ "id", result.id },{ "value", result.value } };

  inputconfig->getInputByName("down", &result);
  player[guid]["down"] = { { "type", input_types[result.type] },{ "id", result.id },{ "value", result.value } };

  inputconfig->getInputByName("left", &result);
  player[guid]["left"] = { { "type", input_types[result.type] },{ "id", result.id },{ "value", result.value } };

  inputconfig->getInputByName("right", &result);
  player[guid]["right"] = { { "type", input_types[result.type] },{ "id", result.id },{ "value", result.value } };

  //inputconfig->getInputByName("leftanalogup", &result);
  player[guid]["analogx"] = { { "type", "axis" },{ "id", 0 },{ "value", 0 } };

  //inputconfig->getInputByName("leftanalogdown", &result);
  player[guid]["analogy"] = { { "type", "axis" },{ "id", 1 },{ "value", 0 } };

  //inputconfig->getInputByName("leftanalogleft", &result);
  player[guid]["analogleft"] = { { "type", "axis" },{ "id", 4 },{ "value", 0 } };

  //inputconfig->getInputByName("leftanalogright", &result);
  player[guid]["analogright"] = { { "type", "axis" },{ "id", 5 },{ "value", 0 } };

  inputconfig->getInputByName("start", &result);
  player[guid]["start"] = { { "type", input_types[result.type] },{ "id", result.id },{ "value", result.value } };

  inputconfig->getInputByName("select", &result);
  player[guid]["select"] = { { "type", input_types[result.type] },{ "id", result.id },{ "value", result.value } };


  return 0;

}

void InputManager::saveInputConfig( const std::string & guid , const std::string & key , const std::string & type, int id , int value){
  json j;
  
  std::ifstream fin( config_fname_ );
  fin >> j;
  fin.close();

  j[guid][key] = { { "type", type },{ "id", id },{ "value", value } };

  std::ofstream out( config_fname_ );
  out << j.dump(2);
  out.close();
}

int InputManager::convertFromEmustationFile( const std::string & fname ){
 
  json joystics;

  // first, open all currently present joysticks
  int numJoysticks = SDL_NumJoysticks();
  for(int i = 0; i < numJoysticks; i++)
  {
    addJoystickByDeviceIndex(i);
  }

  if( mInputConfigs.size() == 0 ){
    
    mKeyboardInputConfig = new InputConfig(DEVICE_KEYBOARD, "Keyboard", KEYBOARD_GUID_STRING);
    if( loadInputConfig(mKeyboardInputConfig) ){
      getPlayerJsonFromInputConfig(0, mKeyboardInputConfig, joystics );
    }else{

      // Set Defalut Key
      joystics["Player1"]["deviceID"]= -1;
      joystics["Player1"]["padmode"] = 0;
      joystics["Player1"]["deviceGUID"] = "-1";
      joystics["Player1"]["deviceName"] = "Keyboard";
      joystics["-1"]["a"] ={ { "type", "key" },{ "id", 122 },{ "value", 0 } };
      joystics["-1"]["b"] ={ { "type", "key" },{ "id", 120 },{ "value", 0 } };
      joystics["-1"]["c"] ={ { "type", "key" },{ "id", 199 },{ "value", 0 } };
      joystics["-1"]["x"] ={ { "type", "key" },{ "id", 97 },{ "value", 0 } };
      joystics["-1"]["y"] ={ { "type", "key" },{ "id", 115 },{ "value", 0 } };
      joystics["-1"]["z"] ={ { "type", "key" },{ "id", 133 },{ "value", 0 } };
      joystics["-1"]["l"] ={ { "type", "key" },{ "id", 101 },{ "value", 0 } };
      joystics["-1"]["r"] ={ { "type", "key" },{ "id", 114 },{ "value", 0 } };
      joystics["-1"]["start"] ={ { "type", "key" },{ "id", 13 },{ "value", 0 } };
      joystics["-1"]["select"] ={ { "type", "key" },{ "id", 93 },{ "value", 0 } };
      joystics["-1"]["up"] ={ { "type", "key" },{ "id", 1073741906 },{ "value", 0 } };
      joystics["-1"]["down"] ={ { "type", "key" },{ "id", 1073741905 },{ "value", 0 } };
      joystics["-1"]["left"] ={ { "type", "key" },{ "id", 1073741904 },{ "value", 0 } };
      joystics["-1"]["right"] ={ { "type", "key" },{ "id", 1073741903 },{ "value", 0 } };
    }

  }else{

    std::map<SDL_JoystickID, InputConfig*>::iterator it;

    //std::vector<json> joystics;
    int i = 0;
    for ( it = mInputConfigs.begin(); it != mInputConfigs.end(); it++ ){
      getPlayerJsonFromInputConfig(i, it->second , joystics );
      i++;
    }
    mKeyboardInputConfig = new InputConfig(DEVICE_KEYBOARD, "Keyboard", KEYBOARD_GUID_STRING);
    if( loadInputConfig(mKeyboardInputConfig) ){
      getPlayerJsonFromInputConfig(i, mKeyboardInputConfig, joystics );
    }
  }

  std::ofstream out(fname);
  out << joystics.dump(2);
  out.close();

  return 0;
}

void InputManager::init( const std::string & fname )
{
  void * padbits;
  Input result;  

  if(initialized())
    deinit();

  SDL_InitSubSystem(SDL_INIT_JOYSTICK);
  SDL_JoystickEventState(SDL_ENABLE);

  json j;

  config_fname_ = fname;
  if(!fs::exists(fname)){
    if( convertFromEmustationFile(fname) != 0 ){
      return;
    }
  }
   
  std::ifstream fin( fname );
  fin >> j;
  std::cout << std::setw(2) << j << "\n\n";

  mapKeys(j);

  try{
    std::string guid = j["player1"]["deviceGUID"];
    select_button_ = j[guid]["select"]["id"];
  }catch( json::exception& e ){
         std::cout << "Select button is not find\n" << "message: " << e.what() << '\n'
                   << "exception id: " << e.id << std::endl;
  }

/*
  // first, open all currently present joysticks
  int numJoysticks = SDL_NumJoysticks();
  for(int i = 0; i < numJoysticks; i++)
  {
    addJoystickByDeviceIndex(i);
  }

  if( numJoysticks != 0 && mInputConfigs.size() != 0 ){
    setGamePadomode( 0, 0 );
    return;
  }

  mKeyboardInputConfig = new InputConfig(DEVICE_KEYBOARD, "Keyboard", KEYBOARD_GUID_STRING);
  loadInputConfig(mKeyboardInputConfig);

  PerPortReset();
  padbits = PerPadAdd(&PORTDATA1);
  mKeyboardInputConfig->getInputByName("up", &result);
  PerSetKey(result.id, PERPAD_UP, padbits);
  mKeyboardInputConfig->getInputByName("down", &result);
  PerSetKey(result.id, PERPAD_DOWN, padbits);
  mKeyboardInputConfig->getInputByName("left", &result);
  PerSetKey(result.id, PERPAD_LEFT, padbits);
  mKeyboardInputConfig->getInputByName("right", &result);
  PerSetKey(result.id, PERPAD_RIGHT, padbits);

  mKeyboardInputConfig->getInputByName("start", &result);
  //printf("start id %d\n",result.id);
  PerSetKey(result.id, PERPAD_START, padbits);

  mKeyboardInputConfig->getInputByName("select", &result);
  select_button_ = result.id;

  mKeyboardInputConfig->getInputByName("a", &result);
  PerSetKey(result.id, PERPAD_A, padbits);

  mKeyboardInputConfig->getInputByName("b", &result);
  PerSetKey(result.id, PERPAD_B, padbits);

  mKeyboardInputConfig->getInputByName("x", &result);
  PerSetKey(result.id, PERPAD_X, padbits);

  mKeyboardInputConfig->getInputByName("y", &result);
  PerSetKey(result.id, PERPAD_Y, padbits);

  mKeyboardInputConfig->getInputByName("rightshoulder", &result);
  PerSetKey(result.id, PERPAD_C, padbits);

  mKeyboardInputConfig->getInputByName("leftshoulder", &result);
  PerSetKey(result.id, PERPAD_Z, padbits);

  mKeyboardInputConfig->getInputByName("lefttrigger", &result);
  PerSetKey(result.id, PERPAD_LEFT_TRIGGER, padbits);

  mKeyboardInputConfig->getInputByName("righttrigger", &result);
  PerSetKey(result.id, PERPAD_RIGHT_TRIGGER, padbits);
*/
}

void InputManager::addJoystickByDeviceIndex(int id)
{
  assert(id >= 0 && id < SDL_NumJoysticks());
  
  // open joystick & add to our list
  SDL_Joystick* joy = SDL_JoystickOpen(id);
  assert(joy);

  // add it to our list so we can close it again later
  SDL_JoystickID joyId = SDL_JoystickInstanceID(joy);
  mJoysticks[joyId] = joy;

  PADLOG("New JoyID = %d id = %d\n",joyId, id);

  char guid[65];
  SDL_JoystickGetGUIDString(SDL_JoystickGetGUID(joy), guid, 65);

  // create the InputConfig
  mInputConfigs[joyId] = new InputConfig(joyId, SDL_JoystickName(joy), guid);
  if(!loadInputConfig(mInputConfigs[joyId]))
  {
    cout << "Added unconfigured joystick " << SDL_JoystickName(joy) << " (GUID: " << guid << ", instance ID: " << joyId << ", device index: " << id << ").";
  }else{
    cout << "Added known joystick " << SDL_JoystickName(joy) << " (instance ID: " << joyId << ", device index: " << id << ")";
  }

  // set up the prevAxisValues
  int numAxes = SDL_JoystickNumAxes(joy);
  mPrevAxisValues[joyId] = new int[numAxes];
  std::fill(mPrevAxisValues[joyId], mPrevAxisValues[joyId] + numAxes, 0); //initialize array to 0

  int numHats = SDL_JoystickNumHats(joy);
  mHatValues[joyId] = new u8[numHats];
  std::fill(mHatValues[joyId], mHatValues[joyId] + numHats, 0); //initialize array to 0

}

void InputManager::removeJoystickByJoystickID(SDL_JoystickID joyId)
{
  assert(joyId != -1);

  // delete old prevAxisValues
  auto axisIt = mPrevAxisValues.find(joyId);
  delete[] axisIt->second;
  mPrevAxisValues.erase(axisIt);

  // delete old InputConfig
  auto it = mInputConfigs.find(joyId);
  delete it->second;
  mInputConfigs.erase(it);

  // close the joystick
  auto joyIt = mJoysticks.find(joyId);
  if(joyIt != mJoysticks.end())
  {
    SDL_JoystickClose(joyIt->second);
    mJoysticks.erase(joyIt);
  }else{
//    LOG(LogError) << "Could not find joystick to close (instance ID: " << joyId << ")";
  }
}

void InputManager::deinit()
{
  if(!initialized())
    return;

  for(auto iter = mJoysticks.begin(); iter != mJoysticks.end(); iter++)
  {
    SDL_JoystickClose(iter->second);
  }
  mJoysticks.clear();

  for(auto iter = mInputConfigs.begin(); iter != mInputConfigs.end(); iter++)
  {
    delete iter->second;
  }
  mInputConfigs.clear();

  for(auto iter = mPrevAxisValues.begin(); iter != mPrevAxisValues.end(); iter++)
  {
    delete[] iter->second;
  }
  mPrevAxisValues.clear();

  if(mKeyboardInputConfig != NULL)
  {
    delete mKeyboardInputConfig;
    mKeyboardInputConfig = NULL;
  }

  SDL_JoystickEventState(SDL_DISABLE);
  SDL_QuitSubSystem(SDL_INIT_JOYSTICK);
}




int InputManager::getNumJoysticks() { return mJoysticks.size(); }
int InputManager::getButtonCountByDevice(SDL_JoystickID id)
{
  if(id == DEVICE_KEYBOARD)
    return 120; //it's a lot, okay.
  else
    return SDL_JoystickNumButtons(mJoysticks[id]);
}

InputConfig* InputManager::getInputConfigByDevice(int device)
{
  if(device == DEVICE_KEYBOARD)
    return mKeyboardInputConfig;
  else
    return mInputConfigs[device];
}

int InputManager::handleJoyEventsMenu(void) {

  int i;
  int j;
  //SDL_Joystick* joy;
  Sint16 cur;
  Uint8 buttonState;
  Uint8 newHatState;
  Uint8 oldHatState;
  int hatValue;

  
  for( auto it = mJoysticks.begin(); it != mJoysticks.end() ; ++it ) {

    SDL_Joystick* joy = it->second;
    SDL_JoystickID joyId = SDL_JoystickInstanceID(joy);
    char guid[65];
    SDL_JoystickGetGUIDString(SDL_JoystickGetGUID(joy), guid, 65);


    for ( i = 0; i < SDL_JoystickNumAxes( joy ); i++ )
    {
      cur = SDL_JoystickGetAxis( joy, i );
      if ( cur < -SDL_MEDIUM_AXIS_VALUE )
      {
        //menu_layer_->onRawInputEvent(*this, guid, "analog", i, -1);
      }
      else if ( cur > SDL_MEDIUM_AXIS_VALUE )
      {
        menu_layer_->onRawInputEvent(*this, guid, "analog", i, 1);
      }      
    }
    
    for ( i = 0; i < SDL_JoystickNumHats( joy ); i++ )
    {
      newHatState = SDL_JoystickGetHat( joy, i );
      oldHatState = mHatValues[ joyId ][ i ];

      for ( j = 0 ; j < SDL_HAT_VALUES_NUM; j++ )
      {
        hatValue = SDL_HAT_VALUES[ j ];
        if ( oldHatState & hatValue && ~newHatState & hatValue )
        {
        }
      }
      for ( j = 0 ; j < SDL_HAT_VALUES_NUM; j++ )
      {
        hatValue = SDL_HAT_VALUES[ j ];
        if ( ~oldHatState & hatValue && newHatState & hatValue )
        {
          if( menu_layer_->onRawInputEvent(*this, guid, "hat", i, hatValue) != -1 ){
            return 0;
          }
        }
      }
      mHatValues[ joyId ][ i ] = newHatState;
    }
  }
  return 0;
}



int InputManager::handleJoyEvents(void) {
  int i;
  int j;
  //SDL_Joystick* joy;
  Sint16 cur;
  Uint8 buttonState;
  Uint8 newHatState;
  Uint8 oldHatState;
  int hatValue;
  
  // update joysticks states
  SDL_JoystickUpdate();

  if(this->menu_layer_) {
    return handleJoyEventsMenu();
  }

  // check each joysticks
  for( auto it = mJoysticks.begin(); it != mJoysticks.end() ; ++it ) {
    
    SDL_Joystick* joy = it->second;
    SDL_JoystickID joyId = SDL_JoystickInstanceID(joy);

    // check axis
    for ( i = 0; i < SDL_JoystickNumAxes( joy ); i++ )
    {
      cur = SDL_JoystickGetAxis( joy, i );
      
      PerAxisValue((joyId << 18) | SDL_MEDIUM_AXIS_VALUE | i, (u8)(((int)cur+32768) >> 8));
      
      if ( cur < -SDL_MEDIUM_AXIS_VALUE )
      {
        PerKeyUp( (joyId << 18) | SDL_MAX_AXIS_VALUE | i );
        PerKeyDown( (joyId << 18) | SDL_MIN_AXIS_VALUE | i );
        //printf("PerAxisValue %d, %d\n",i,((u8)(((int)cur+32768) >> 8)));
      }
      else if ( cur > SDL_MEDIUM_AXIS_VALUE )
      {
        PerKeyUp( (joyId << 18) | SDL_MIN_AXIS_VALUE | i );
        PerKeyDown( (joyId << 18) | SDL_MAX_AXIS_VALUE | i );
        //printf("PerAxisValue %d, %d\n",i,((u8)(((int)cur+32768) >> 8)));
      }
      else
      {
        PerKeyUp( (joyId << 18) | SDL_MIN_AXIS_VALUE | i );
        PerKeyUp( (joyId << 18) | SDL_MAX_AXIS_VALUE | i );
      }
    }
    
    // check buttons
    for ( i = 0; i < SDL_JoystickNumButtons( joy ); i++ )
    {
      buttonState = SDL_JoystickGetButton( joy, i );
      
      if ( buttonState == SDL_BUTTON_PRESSED )
      {
        PerKeyDown( (joyId << 18) | (i) );
        //printf("SDL_BUTTON_PRESSED %d",(i));
      }
      else if ( buttonState == SDL_BUTTON_RELEASED )
      {
        PerKeyUp( (joyId << 18) | (i) );
        //printf("SDL_BUTTON_RELEASED %d\n",(i +1));
      }
    }

    // check hats
    for ( i = 0; i < SDL_JoystickNumHats( joy ); i++ )
    {
      newHatState = SDL_JoystickGetHat( joy, i );
      oldHatState = mHatValues[ joyId ][ i ];

      for ( j = 0 ; j < SDL_HAT_VALUES_NUM; j++ )
      {
        hatValue = SDL_HAT_VALUES[ j ];
        if ( oldHatState & hatValue && ~newHatState & hatValue )
        {
          PerKeyUp( (joyId << 18) | SDL_HAT_VALUE | (hatValue << 4) | i );
        }
      }
      for ( j = 0 ; j < SDL_HAT_VALUES_NUM; j++ )
      {
        hatValue = SDL_HAT_VALUES[ j ];
        if ( ~oldHatState & hatValue && newHatState & hatValue )
        {
          //printf("HAT %d \n", hatValue);
          PerKeyDown( (joyId << 18) | SDL_HAT_VALUE | (hatValue << 4) | i);
        }
      }

      mHatValues[ joyId ][ i ] = newHatState;
    }
  }

  // return success
  return 0;
}

static const int DEADZONE = 23000;

void InputManager::setMenuLayer( MenuScreen * menu_layer ){
  menu_layer_ = menu_layer;
  if( menu_layer_ != nullptr ){
    menu_layer_->setCurrentInputDevices( mJoysticks );
  }
}

bool InputManager::parseEventMenu(const SDL_Event& ev ){
  std::vector<std::string> evstr;
  switch(ev.type)
  {
  case SDL_JOYAXISMOTION:{
    //if it switched boundaries
    bool causedEvent = false;
    if((abs(ev.jaxis.value) > DEADZONE) != (abs(mPrevAxisValues[ev.jaxis.which][ev.jaxis.axis]) > DEADZONE)) {
      int normValue;
      if(abs(ev.jaxis.value) <= DEADZONE)
        normValue = 0;
      else
        if(ev.jaxis.value > 0)
          normValue = 1;
        else
          normValue = -1;
      //window->input(getInputConfigByDevice(ev.jaxis.which), Input(ev.jaxis.which, TYPE_AXIS, ev.jaxis.axis, normValue, false));
      InputConfig* cfg = getInputConfigByDevice(ev.jbutton.which);
      evstr = cfg->getMappedTo(Input(ev.jbutton.which, TYPE_BUTTON, ev.jbutton.button, normValue, false));
      if( evstr.size() > 0 ){
        menu_layer_->keyboardEvent(evstr[0],0,normValue,0);
      }
      causedEvent = true;
    }
    mPrevAxisValues[ev.jaxis.which][ev.jaxis.axis] = ev.jaxis.value;
    return causedEvent;    
  }
  case SDL_JOYBUTTONDOWN:
  case SDL_JOYBUTTONUP:{
      InputConfig* cfg = getInputConfigByDevice(ev.jbutton.which);
      evstr = cfg->getMappedTo(Input(ev.jbutton.which, TYPE_BUTTON, ev.jbutton.button, ev.jbutton.state == SDL_PRESSED, false));
      if( evstr.size() ) {
        menu_layer_->keyboardEvent(evstr[0],0,ev.jbutton.state == SDL_PRESSED,0);
      }
      {
        char guid[65];
        SDL_JoystickGetGUIDString(SDL_JoystickGetGUID(mJoysticks[ev.jbutton.which]), guid, 65);
        if( SDL_JOYBUTTONDOWN == ev.type ){
          if( menu_layer_->onRawInputEvent(*this,guid,"button",ev.jbutton.button,1) != -1 ){
            return true;
          }
        }
      }
    if( SDL_JOYBUTTONDOWN == ev.type && (ev.jbutton.button == select_button_ ||evstr[0]=="b")  ){
      printf("press back\n");
      if( menu_layer_->onBackButtonPressed() == 0 ){
        SDL_Event event = {};
        event.type = showmenu_;
        event.user.code = 0;
        event.user.data1 = 0;
        event.user.data2 = 0;
        SDL_PushEvent(&event);
      }
    }
      return true;
  }
  break;
  case SDL_JOYHATMOTION:{
      InputConfig* cfg = getInputConfigByDevice(ev.jhat.which);
      evstr = cfg->getMappedTo( Input(ev.jhat.which, TYPE_HAT, ev.jhat.hat, ev.jhat.value, false));
      if( evstr.size() != 0 ){
        menu_layer_->keyboardEvent(evstr[0],0, (ev.jhat.value != 0) ,0);
      }
      return true;
  }
  break;
  case SDL_KEYDOWN:
    if(ev.key.repeat)
      return false;  

    menu_layer_->onRawInputEvent(*this,"-1", "key",ev.key.keysym.sym,1);

    if( mKeyboardInputConfig )  {
      evstr = mKeyboardInputConfig->getMappedTo(Input(DEVICE_KEYBOARD, TYPE_KEY, ev.key.keysym.sym, 1, false));
      if( evstr.size() != 0 ){
        menu_layer_->keyboardEvent(evstr[0],0,1,0);
      }
      return true;
    }else{
      return false;
    }

    break;
  case SDL_KEYUP:
    if( mKeyboardInputConfig )  {
      evstr = mKeyboardInputConfig->getMappedTo(Input(DEVICE_KEYBOARD, TYPE_KEY, ev.key.keysym.sym, 0, false));
      if( evstr.size() != 0 ){
        menu_layer_->keyboardEvent(evstr[0],0,0,0);
      }
      return true;
    }else{
      return false;
    }
    break;
  }
  return false;
}

bool InputManager::parseEvent(const SDL_Event& ev)
{
  if( this->menu_layer_) {
    return parseEventMenu(ev);
  }
  bool causedEvent = false;
  switch(ev.type)
  {
  case SDL_JOYBUTTONUP:{
    if( ev.jbutton.button == select_button_ ){
      SDL_Event event = {};
      event.type = showmenu_;
      event.user.code = 0;
      event.user.data1 = 0;
      event.user.data2 = 0;
      SDL_PushEvent(&event);
    }
    return true;
  }    
  case SDL_KEYDOWN:
    if(ev.key.repeat)
      return false;
    if(ev.key.keysym.sym == SDLK_ESCAPE)
    {
      SDL_Event event = {};
      event.type = showmenu_;
      event.user.code = 0;
      event.user.data1 = 0;
      event.user.data2 = 0;
      SDL_PushEvent(&event);
      return false;
    }
    PerKeyDown( ev.key.keysym.sym | KEYBOARD_MASK );
    //printf("PerKeyDown %d\n", ev.key.keysym.sym);
    return true;
  case SDL_KEYUP:
     PerKeyUp( ev.key.keysym.sym | KEYBOARD_MASK );
    return true;
  case SDL_JOYDEVICEADDED:
    updateConfig();
    addJoystickByDeviceIndex(ev.jdevice.which); // ev.jdevice.which is a device index
    return true;
  case SDL_JOYDEVICEREMOVED:
    updateConfig();  
    removeJoystickByJoystickID(ev.jdevice.which); // ev.jdevice.which is an SDL_JoystickID (instance ID)
    return false;
  }
  return false;
}

bool InputManager::loadInputConfig(InputConfig* config)
{
  std::string path = getConfigPath();
  if(!fs::exists(path)){
    PADLOG("loadInputConfig %s is not found!\n", path.c_str() );
    return false;
  }
    
  pugi::xml_document doc;
  pugi::xml_parse_result res = doc.load_file(path.c_str());

  if(!res)
  {
//    LOG(LogError) << "Error parsing input config: " << res.description();
    return false;
  }

  pugi::xml_node root = doc.child("inputList");
  if(!root)
    return false;

  pugi::xml_node configNode = root.find_child_by_attribute("inputConfig", "deviceGUID", config->getDeviceGUIDString().c_str());
  if(!configNode)
    configNode = root.find_child_by_attribute("inputConfig", "deviceName", config->getDeviceName().c_str());
  if(!configNode)
    return false;

  config->loadFromXML(configNode);
  return true;
}

//used in an "emergency" where no keyboard config could be loaded from the inputmanager config file
//allows the user to select to reconfigure in menus if this happens without having to delete es_input.cfg manually
void InputManager::loadDefaultKBConfig()
{
  InputConfig* cfg = getInputConfigByDevice(DEVICE_KEYBOARD);

  cfg->clear();
  cfg->mapInput("up", Input(DEVICE_KEYBOARD, TYPE_KEY, SDLK_UP, 1, true));
  cfg->mapInput("down", Input(DEVICE_KEYBOARD, TYPE_KEY, SDLK_DOWN, 1, true));
  cfg->mapInput("left", Input(DEVICE_KEYBOARD, TYPE_KEY, SDLK_LEFT, 1, true));
  cfg->mapInput("right", Input(DEVICE_KEYBOARD, TYPE_KEY, SDLK_RIGHT, 1, true));

  cfg->mapInput("a", Input(DEVICE_KEYBOARD, TYPE_KEY, SDLK_RETURN, 1, true));
  cfg->mapInput("b", Input(DEVICE_KEYBOARD, TYPE_KEY, SDLK_ESCAPE, 1, true));
  cfg->mapInput("start", Input(DEVICE_KEYBOARD, TYPE_KEY, SDLK_F1, 1, true));
  cfg->mapInput("select", Input(DEVICE_KEYBOARD, TYPE_KEY, SDLK_F2, 1, true));

  cfg->mapInput("pageup", Input(DEVICE_KEYBOARD, TYPE_KEY, SDLK_RIGHTBRACKET, 1, true));
  cfg->mapInput("pagedown", Input(DEVICE_KEYBOARD, TYPE_KEY, SDLK_LEFTBRACKET, 1, true));
}

void InputManager::writeDeviceConfig(InputConfig* config)
{
  assert(initialized());

  std::string path = getConfigPath();

  pugi::xml_document doc;

  if(fs::exists(path))
  {
    // merge files
    pugi::xml_parse_result result = doc.load_file(path.c_str());
    if(!result)
    {
//      LOG(LogError) << "Error parsing input config: " << result.description();
    }else{
      // successfully loaded, delete the old entry if it exists
      pugi::xml_node root = doc.child("inputList");
      if(root)
      {
        pugi::xml_node oldEntry = root.find_child_by_attribute("inputConfig", "deviceGUID", config->getDeviceGUIDString().c_str());
        if(oldEntry)
          root.remove_child(oldEntry);
        oldEntry = root.find_child_by_attribute("inputConfig", "deviceName", config->getDeviceName().c_str());
        if(oldEntry)
          root.remove_child(oldEntry);
      }
    }
  }

  pugi::xml_node root = doc.child("inputList");
  if(!root)
    root = doc.append_child("inputList");

  config->writeToXML(root);
  doc.save_file(path.c_str());
}

std::string InputManager::getConfigPath()
{
  std::string path = getenv("HOME");
  path += "/.emulationstation/es_temporaryinput.cfg";
  return path;
}

bool InputManager::initialized() const
{
  return mKeyboardInputConfig != NULL;
}

int InputManager::getNumConfiguredDevices()
{
  int num = 0;
  for(auto it = mInputConfigs.begin(); it != mInputConfigs.end(); it++)
  {
    if(it->second->isConfigured())
      num++;
  }

  if(mKeyboardInputConfig->isConfigured())
    num++;

  return num;
}

std::string InputManager::getDeviceGUIDString(int deviceId)
{
  if(deviceId == DEVICE_KEYBOARD)
    return KEYBOARD_GUID_STRING;

  auto it = mJoysticks.find(deviceId);
  if(it == mJoysticks.end())
  {
//    LOG(LogError) << "getDeviceGUIDString - deviceId " << deviceId << " not found!";
    return "something went horribly wrong";
  }

  char guid[65];
  SDL_JoystickGetGUIDString(SDL_JoystickGetGUID(it->second), guid, 65);
  return std::string(guid);
}