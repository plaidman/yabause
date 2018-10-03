/*  Copyright 2018 devMiyax(smiyaxdev@gmail.com)

This file is part of YabaSanshiro.

Yabause is free software; you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation; either version 2 of the License, or
(at your option) any later version.

Yabause is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with YabaSanshiro; if not, write to the Free Software
Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301  USA
*/
#include "MenuScreen.h"
#include <nanogui/window.h>
#include <nanogui/layout.h>
#include <nanogui/label.h>
#include <nanogui/button.h>
#include <nanogui/popupbutton.h>
#include <nanogui/messagedialog.h>
#include <nanogui/combobox.h>
#include "../config.h"
#include <iostream>
#include <fstream>
#include <sstream>

#include "InputManager.h"

//#define MENU_LOG
#define MENU_LOG printf

using namespace std;

int MenuScreen::onShow(){
  //setupPlayerPsuhButton( 0, player1, "Player1 Input Settings", &p1cb );
  //setupPlayerPsuhButton( 1, player2, "Player2 Input Settings", &p2cb );
}

MenuScreen::MenuScreen( SDL_Window* pwindow, int rwidth, int rheight, const std::string & fname )
: nanogui::Screen( pwindow, Eigen::Vector2i(rwidth, rheight), "Menu Screen"){
  
  mFocus = nullptr;
  config_file_ = fname;
  swindow = nullptr;
  std::string title = "Yaba Sanshiro "+ std::string(YAB_VERSION) +" Menu";
        window = new Window(this, title);
        window->setPosition(Vector2i(0, 0));
        window->setLayout(new GroupLayout());

        tools = new Widget(window);
        pushActiveMenu(tools, nullptr );
        tools->setLayout(new BoxLayout(Orientation::Vertical,Alignment::Middle, 0, 5));
        tools->setFixedWidth(256);

        player1 = new PopupButton(tools, "Player1", ENTYPO_ICON_EXPORT);      
        setupPlayerPsuhButton( 0, player1, "Player1 Input Settings", &p1cb );

        player2 = new PopupButton(tools, "Player2", ENTYPO_ICON_EXPORT);    
        setupPlayerPsuhButton( 1, player2, "Player2 Input Settings", &p2cb );

        Button *b0 = new Button(tools, "Exit");
        b0->setFixedWidth(248);
        b0->setCallback([this]() { 
          MENU_LOG("Exit\n"); 
      		SDL_Event* quit = new SDL_Event();
			    quit->type = SDL_QUIT;
			    SDL_PushEvent(quit);  
        });

        Button *b1 = new Button(tools, "Reset");
        b1->setFixedWidth(248);
        b1->setCallback([this]() { 
          MENU_LOG("Reset\n");  
          SDL_Event event = {};
          event.type = reset_;
          event.user.code = 0;
          event.user.data1 = 0;
          event.user.data2 = 0;
          SDL_PushEvent(&event);          
        });

        Button *b2 = new Button(tools, "Show/Hide FPS");
        b2->setFixedWidth(248);
        b2->setCallback([this]() { 
          MENU_LOG("Reset\n");  
          SDL_Event event = {};
          event.type = toggile_fps_;
          event.user.code = 0;
          event.user.data1 = 0;
          event.user.data2 = 0;
          SDL_PushEvent(&event);          
        });

        Button *b3 = new Button(tools, "Enable/Disable Frame Skip");
        b3->setFixedWidth(248);
        b3->setCallback([this]() { 
          MENU_LOG("Reset\n");  
          SDL_Event event = {};
          event.type = toggile_frame_skip_;
          event.user.code = 0;
          event.user.data1 = 0;
          event.user.data2 = 0;
          SDL_PushEvent(&event);          
        });        

        //Button *b4 = new Button(tools, "Open CD Tray");
        //b4->setFixedWidth(120);
        //b4->setCallback([this]() { printf("test4");  });
/*
        bAnalog = new Button(tools, "Switch Pad Mode");
        bAnalog->setFixedWidth(248);
        bAnalog->setCallback([this]() { 
          SDL_Event event = {};
          event.type = pad_;
          event.user.code = 0;
          event.user.data1 = 0;
          event.user.data2 = 0;
          SDL_PushEvent(&event);            
        });
*/
        player1->focusEvent(true);
        player1->mouseEnterEvent(player1->absolutePosition(),true);
        mFocus = player1;
        performLayout();
}

void MenuScreen::showInputCheckDialog( const std::string & key ){

    swindow = new Window(this, key);
    swindow->setPosition(Vector2i(0, 0));
    swindow->setLayout(new GroupLayout(32));
    new Label(swindow,"Push key for " + key, "sans", 64);
    swindow->center();
    swindow->setModal(true);
    swindow->requestFocus();
    current_key_ = key;
}

void MenuScreen::setupPlayerPsuhButton( int user_index, PopupButton *player, const std::string & label, ComboBox **cbo ){
  player->setFixedWidth(248);
  Popup *popup = player->popup();     
  popup->setLayout(new GroupLayout(8,4,4,4)); 
  new Label(popup, label);

  std::string username;


  json j;
  string selguid="BADGUID";
  int padmode=0;
  std::stringstream ss;
  std::string userid;

  try{
    std::ifstream fin( config_file_ );
    fin >> j;
    fin.close();
    ss << "player" << (user_index+1);
    userid = ss.str();
    if( j.find(userid) != j.end() ) {
      selguid = j[userid]["deviceGUID"];
      padmode = j[userid]["padmode"];
    }
  }catch ( json::exception& e ){

  }

  ComboBox * cb = new ComboBox(popup);  
  vector<string> items;
  vector<string> itemsShort;
  int index = 0;
  int selindex = -1;
  for( auto it = joysticks_.begin(); it != joysticks_.end() ; ++it ) {
    SDL_Joystick* joy = it->second;
    SDL_JoystickID joyId = SDL_JoystickInstanceID(joy);
    char guid[65];
    SDL_JoystickGetGUIDString(SDL_JoystickGetGUID(joy), guid, 65);
    if( selguid  == string("guid") ){
      selindex = index;
    }
    items.push_back(guid);
    itemsShort.push_back(SDL_JoystickName(joy));
  }  
  cb->setItems(itemsShort);

  if( selindex != -1 ){
    cb->setSelectedIndex(selindex);
  }

  Popup *cbpopup = cb->popup(); 
  cb->setCallback([this,cbpopup,cb]() {       
    pushActiveMenu(cbpopup, cb );
  });
  
  cb->setCallbackSelect([this, userid]( int idx ) {
      popActiveMenu();

      SDL_JoystickID joyId = -1;
      int itenindex = 0;
      std::string device_name = "Keyboard";
      if( idx >= joysticks_.size() ){
        cuurent_deviceguid_ = "-1"; // keyboard may be
      }else{
        for( auto it = joysticks_.begin(); it != joysticks_.end() ; ++it ) {
          if( itenindex == idx ){
            SDL_Joystick* joy = it->second;
            joyId = SDL_JoystickInstanceID(joy);
            char guid[65];
            SDL_JoystickGetGUIDString(SDL_JoystickGetGUID(joy), guid, 65);
            cuurent_deviceguid_ = guid;
            device_name = SDL_JoystickName(joy);
            break;
          }
          itenindex++;
        }
      }

      json j;
      std::ifstream fin( this->config_file_ );
      fin >> j;
      fin.close();
      //if( j.find(userid) == j.end() ) {
      //  return;
      //}            
      j[userid]["deviceGUID"] = cuurent_deviceguid_;
      j[userid]["DeviceID"]   = joyId;
      j[userid]["deviceName"] = device_name;
      std::ofstream out(this->config_file_);
      out << j.dump(2);
      out.close();        

      SDL_Event event = {};
      event.type = this->update_config_;
      event.user.code = 0;
      event.user.data1 = 0;
      event.user.data2 = 0;
      SDL_PushEvent(&event);         

  });


  *cbo = cb;

  Button * ba = new Button(popup,"Analog mode");  
  ba->setFlags(Button::ToggleButton); 
  if( padmode != 0 ){
    ba->setPushed(true);
  }else{
    ba->setPushed(false);
  }
  ba->setChangeCallback([this, userid](bool state) { 
    cout << "Toggle button state: " << state << endl;

    json j;
    std::ifstream fin( this->config_file_ );
    fin >> j;
    fin.close();
    //if( j.find(userid) == j.end() ) {
    //  return;
    //}

    try{
      if( state )
        j[userid]["padmode"] = 1;
      else
        j[userid]["padmode"] = 0;

      std::ofstream out(this->config_file_);
      out << j.dump(2);
      out.close();

      SDL_Event event = {};
      event.type = this->update_config_;
      event.user.code = 0;
      event.user.data1 = 0;
      event.user.data2 = 0;
      SDL_PushEvent(&event);   
    }catch ( json::exception& e ){

    }

  }); 

  
  player->setCallback([this,popup,player,user_index]() {      
    pushActiveMenu(popup,player); 
    std::stringstream s ;
    s << "player" << user_index;
    current_user_ = s.str();

  });


  Button * b;
  b = new Button(popup, "UP");
  b->setCallback([this, selguid]{
    this->cuurent_deviceguid_ = selguid;
    showInputCheckDialog("up");
  });
  b = new Button(popup, "DOWN");
  b->setCallback([this, selguid]{
    this->cuurent_deviceguid_ = selguid;
    showInputCheckDialog("down");
  });
  b = new Button(popup, "LEFT");
  b->setCallback([this, selguid]{
    this->cuurent_deviceguid_ = selguid;
    showInputCheckDialog("left");
  });
  b = new Button(popup, "RIGHT");
  b->setCallback([this, selguid]{
    this->cuurent_deviceguid_ = selguid;
    showInputCheckDialog("right");
  });

  b = new Button(popup, "START");
  b->setCallback([this, selguid]{
    this->cuurent_deviceguid_ = selguid;
    showInputCheckDialog("start");
  });
  b = new Button(popup, "A");
  b->setCallback([this, selguid]{
    this->cuurent_deviceguid_ = selguid;
    showInputCheckDialog("a");
  }); 

  b = new Button(popup, "B");
  b->setCallback([this, selguid]{
    this->cuurent_deviceguid_ = selguid;
    showInputCheckDialog("b");
  }); 

  b = new Button(popup, "C");
  b->setCallback([this, selguid]{
    this->cuurent_deviceguid_ = selguid;
    showInputCheckDialog("c");
  });   
  b = new Button(popup, "X");
  b->setCallback([this, selguid]{
    this->cuurent_deviceguid_ = selguid;
    showInputCheckDialog("x");
  });   
  b = new Button(popup, "Y");
  b->setCallback([this, selguid]{
    this->cuurent_deviceguid_ = selguid;
    showInputCheckDialog("y");
  });     
  b = new Button(popup, "Z");
  b->setCallback([this, selguid]{
    this->cuurent_deviceguid_ = selguid;
    showInputCheckDialog("z");
  });     
  b = new Button(popup, "L");
  b->setCallback([this, selguid]{
    this->cuurent_deviceguid_ = selguid;
    showInputCheckDialog("l");
  });   
  b = new Button(popup, "R");
  b->setCallback([this, selguid]{
    this->cuurent_deviceguid_ = selguid;
    showInputCheckDialog("r");
  });   
  b = new Button(popup, "Analog X");
  b->setCallback([this, selguid]{
    this->cuurent_deviceguid_ = selguid;
    showInputCheckDialog("analogx");
  });   
  b = new Button(popup, "Analog Y");
  b->setCallback([this, selguid]{
    this->cuurent_deviceguid_ = selguid;
    showInputCheckDialog("analogy");
  });   
  b = new Button(popup, "Analog L");
  b->setCallback([this, selguid]{
    this->cuurent_deviceguid_ = selguid;
    showInputCheckDialog("analogl");
  });   
  b = new Button(popup, "Analog R");
  b->setCallback([this, selguid]{
    this->cuurent_deviceguid_ = selguid;
    showInputCheckDialog("analogr");
  });   
}

/*
int MenuScreen::OnInputSelected( string & type, int & id, int & value ){
  current_player[ buttons[ current_setting_index ] ] = { {"id",id},{"value",value},{"type",type}};
  current_setting_index++;

  b
}
*/

int MenuScreen::onRawInputEvent( InputManager & imp, const std::string & deviceguid, const std::string & type, int id, int value ){
  if( swindow == nullptr ){ return -1; }

  cout << "onRawInputEvent deviceguid:" << deviceguid << " type:" << type << " id:" << id << " val:" << value << endl;

  if( deviceguid != cuurent_deviceguid_ ){ 
    cout << "deviceguid = " << deviceguid << " vs cuurent_deviceguid = " << cuurent_deviceguid_ << endl;
    return -1; 
  }

  // wait for key input?
  if( current_key_.find("analog") == std::string::npos ){
    if( type == "axis" ){
      return -1;
    }
  }else{
    if( type != "axis" ){
      return -1;
    }
  }

  imp.saveInputConfig( deviceguid , current_key_, type, id, value);
  swindow->dispose();
  swindow = nullptr;

  SDL_Event event = {};
  event.type = this->update_config_;
  event.user.code = 0;
  event.user.data1 = 0;
  event.user.data2 = 0;
  SDL_PushEvent(&event);   
  return 0;
}

bool MenuScreen::keyboardEvent( std::string & keycode , int scancode, int action, int modifiers){

  if( swindow != nullptr ){ return false; }

  MENU_LOG("%s %d %d\n",keycode.c_str(),scancode,action);
  if (action == 1) {
          
    if (keycode == "down") {
      nanogui::Button* btn = (nanogui::Button*)mFocus;
      //btn->setPushed(false);
      mFocus->mouseEnterEvent(mFocus->position(), false);
      mFocus = getActiveMenu()->getNearestWidget(mFocus, 1);
      MENU_LOG("%s is selected\n",((nanogui::Button*)mFocus)->caption().c_str());
      mFocus->mouseEnterEvent(mFocus->position(), true);
    }else if (keycode == "up") {
      nanogui::Button* btn = (nanogui::Button*)mFocus;
      //btn->setPushed(false);
      mFocus->mouseEnterEvent(mFocus->position(), false);
      mFocus = getActiveMenu()->getNearestWidget(mFocus, 0);
      mFocus->mouseEnterEvent(mFocus->position(), true);
    }
    if (keycode == "a") {
        mFocus->mouseButtonEvent(mFocus->position(), SDL_BUTTON_LEFT, true, 0);
    }
  }
 
  if (action == 0 && keycode == "a" ) {
    mFocus->mouseButtonEvent(mFocus->position(), SDL_BUTTON_LEFT, false, 0);
  }    

  return false;
}

void MenuScreen::setBackGroundImage( const std::string & fname ){
  if(imageid_!=0){
    nvgDeleteImage(mNVGContext,imageid_);
  }
  imageid_ = nvgCreateImage(mNVGContext, fname.c_str(), 0 );
  nvgImageSize(mNVGContext, imageid_, &imgw_, &imgh_);
  MENU_LOG("imageid_:%d w:%d h:%d\n",imageid_,imgw_,imgh_);
	imgPaint_ = nvgImagePattern(mNVGContext, 0, 0, imgw_,imgh_, 0, imageid_, 0.5f);
}

void MenuScreen::draw(NVGcontext *ctx){

  if( imageid_ != 0 ){
    nvgSave(ctx);
		nvgBeginPath(ctx);
		nvgRect(ctx, 0, 0, imgw_,imgh_);
		nvgFillPaint(ctx, imgPaint_);
    //nvgFillColor(ctx, nvgRGBA(0,160,192,255));
		nvgFill(ctx);
    nvgRestore(ctx);
  }

  //window->center();
  Screen::draw(ctx);
}

void MenuScreen::setCurrentInputDevices( std::map<SDL_JoystickID, SDL_Joystick*> & joysticks ){

  joysticks_ = joysticks;
  
  vector<string> items;
  vector<string> itemsShort;

  json j;
  string selguid="BADGUID";
  std::stringstream ss;
  std::string userid;

  try{
    std::ifstream fin( config_file_ );
    fin >> j;
    fin.close();
  }catch ( json::exception& e ){

  }


  for( auto it = joysticks_.begin(); it != joysticks_.end() ; ++it ) {
      SDL_Joystick* joy = it->second;
      SDL_JoystickID joyId = SDL_JoystickInstanceID(joy);
      char guid[65];
      SDL_JoystickGetGUIDString(SDL_JoystickGetGUID(joy), guid, 65);
      items.push_back(guid);
      itemsShort.push_back(SDL_JoystickName(joy));
  }  
  itemsShort.push_back("KeyBoard");
  items.push_back("-1");
  itemsShort.push_back("Disable");
  items.push_back("-2");


  if( p1cb != nullptr ) { 
    p1cb->setItems(itemsShort,itemsShort); 
    userid = "player1";
    if( j.find(userid) != j.end() ) {
      selguid = j[userid]["deviceGUID"];
    }else{
      selguid = "-2";
    }
    for( int i=0; i<items.size(); i++  ){
       if( items[i] == selguid ){
         p1cb->setSelectedIndex(i);
         break;
       }
    }
  }

  if( p2cb != nullptr ) { 
    p2cb->setItems(itemsShort,itemsShort); 
    userid = "player2";
    if( j.find(userid) != j.end() ) {
      selguid = j[userid]["deviceGUID"];
    }else{
      selguid = "-2";
    }
    for( int i=0; i<items.size(); i++  ){
      if( items[i] == selguid ){
        p2cb->setSelectedIndex(i);
        break;
      }
    }
  }
  performLayout();
}

void MenuScreen::pushActiveMenu( Widget *active, Widget * button  ){
    PreMenuInfo tmp;
    tmp.window = active;
    tmp.button = button;
    menu_stack.push(tmp);
    if( mFocus != nullptr ){
      mFocus->mouseEnterEvent(mFocus->position(), false);
      const std::vector<Widget *> &children = active->children();
      for( int i=0; i<children.size(); i++ ){
        if(  dynamic_cast<Button*>(children[i]) != nullptr ){
          mFocus = children[i];
          mFocus->requestFocus();
          MENU_LOG("%s is selected\n",((nanogui::Button*)mFocus)->caption().c_str());
          mFocus->mouseEnterEvent(mFocus->position(), true);
          break;
        }
      }
  }
}

void MenuScreen::popActiveMenu(){
  MENU_LOG("popActiveMenu");
  if( menu_stack.size() <= 1 ) return;
  PreMenuInfo tmp;
  tmp = menu_stack.top();
  PopupButton * p = dynamic_cast<PopupButton*>(tmp.button);
  if( p != nullptr ) {
    MENU_LOG("Hide menu\n");
    mFocus->mouseEnterEvent(p->absolutePosition(),false);
    p->setPushed(false);
    p->popup()->setVisible(false);
    p->mouseEnterEvent(p->absolutePosition(),true);
    mFocus = p;
    mFocus->requestFocus();
    menu_stack.pop();
  }else{
    menu_stack.pop();
    tmp = menu_stack.top();
    const std::vector<Widget *> &children = tmp.window->children();
    for( int i=0; i<children.size(); i++ ){
      if(  dynamic_cast<Button*>(children[i]) != nullptr ){
        Widget * px = children[i];
        mFocus->mouseEnterEvent(p->absolutePosition(),false);
        px->mouseEnterEvent(p->absolutePosition(),true);
        mFocus = px;
        mFocus->requestFocus();        
        break;
      }
    }
  }
  return;
}

Widget * MenuScreen::getActiveMenu(){
  PreMenuInfo tmp;
  tmp = menu_stack.top();
  return tmp.window;
}

int MenuScreen::onBackButtonPressed(){
  MENU_LOG("onBackButtonPressed\n");
  /*
  if( swindow != nullptr ){
    swindow->dispose();>setVisible(false);
    swindow = nullptr;
    return 1;    
  }
  */
 if( swindow != nullptr ){ 
   printf("swindow != null\n");
   return 1; 
 }


  Widget * item = getActiveMenu();
  if( item == tools ){
    printf("Finish\n");
    return 0;
  }
  popActiveMenu();
/*
  if( activeWindow == tools ){
    return 0;
  }
  player1->setPushed(false);
  player2->setPushed(false);
  activeWindow = tools;
  Widget * p = tools->children()[0];
  if( p != nullptr ) {
    p->mouseEnterEvent(p->absolutePosition(),true);
    mFocus = p;
    mFocus->focusEvent(true);
  }
*/  
  return 1;
}
