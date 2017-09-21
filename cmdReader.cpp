/****************************************************************************
  FileName     [ cmdReader.cpp ]
  PackageName  [ cmd ]
  Synopsis     [ Define command line reader member functions ]
  Author       [ Chung-Yang (Ric) Huang ]
  Copyright    [ Copyleft(c) 2007-present LaDs(III), GIEE, NTU, Taiwan ]
  Student      [ b05901033 Jexus Chuang ]
****************************************************************************/
#include <cassert>
#include <cstring>
#include "cmdParser.h"
#include <string>
#include <sys/ioctl.h>
#include <sstream>

using namespace std;

//----------------------------------------------------------------------
//    Extrenal funcitons
//----------------------------------------------------------------------
void mybeep();
char mygetc(istream&);
ParseChar getChar(istream&);


//----------------------------------------------------------------------
//    Member Function for class Parser
//----------------------------------------------------------------------
void
CmdParser::readCmd()
{
   if (_dofile.is_open()) {
      readCmdInt(_dofile);
      _dofile.close();
   }
   else
      readCmdInt(cin);
}

void
CmdParser::readCmdInt(istream& istr)
{
   resetBufAndPrintPrompt();
   struct winsize w;
   ioctl(0, TIOCGWINSZ, &w);
   int cover = w.ws_col - 5;
   while (1) {
      ParseChar pch = getChar(istr);
      if (pch == INPUT_END_KEY) break;
      switch (pch) {
         case LINE_BEGIN_KEY :
         case HOME_KEY       : moveBufPtr(_readBuf); break;
         case LINE_END_KEY   :
         case END_KEY        : moveBufPtr(_readBufEnd); break;
         case BACK_SPACE_KEY : if(moveBufPtr(_readBufPtr-1)) deleteChar(); break;
         case DELETE_KEY     : deleteChar(); break;
         case NEWLINE_KEY    : addHistory();
                               cout << char(NEWLINE_KEY);
                               resetBufAndPrintPrompt(); break;
         case ARROW_UP_KEY   : moveToHistory(_historyIdx - 1); break;
         case ARROW_DOWN_KEY : moveToHistory(_historyIdx + 1); break;
         case ARROW_RIGHT_KEY: moveBufPtr(_readBufPtr+1); break;
         case ARROW_LEFT_KEY : moveBufPtr(_readBufPtr-1); break;
         case PG_UP_KEY      : moveToHistory(_historyIdx - PG_OFFSET); break;
         case PG_DOWN_KEY    : moveToHistory(_historyIdx + PG_OFFSET); break;
         case TAB_KEY        : insertChar(' ',TAB_POSITION); break;
         case INSERT_KEY     : // not yet supported; fall through to UNDEFINE
         case UNDEFINED_KEY:   mybeep(); break;
         default:  // printable character
            insertChar(char(pch));
            break;
      }
      cout << '\r' << "cmd> ";
      for(int i=0; i<cover; i++){
          cout << ' ';
      }
      cout << '\r' << "cmd> ";
      for(char* i=_readBuf; i<_readBufEnd; i++){
          cout << *i ;
      }
      cout << '\r' << "cmd> ";
      for(char* i=_readBuf; i<_readBufPtr; i++){
          cout << *i ;
      }
      #ifdef TA_KB_SETTING
      taTestOnly();
      #endif
   }
}


// This function moves _readBufPtr to the "ptr" pointer
// It is used by left/right arrowkeys, home/end, etc.
//
// Suggested steps:
// 1. Make sure ptr is within [_readBuf, _readBufEnd].
//    If not, make a beep sound and return false. (DON'T MOVE)
// 2. Move the cursor to the left or right, depending on ptr
// 3. Update _readBufPtr accordingly. The content of the _readBuf[] will
//    not be changed
//
// [Note] This function can also be called by other member functions below
//        to move the _readBufPtr to proper position.
bool
CmdParser::moveBufPtr(char* const ptr)
{
   // TODO...
   if(!(_readBuf <= ptr && ptr <= _readBufEnd)){
       mybeep();
       return false;
   }
   _readBufPtr = ptr;
       return true;
}


// [Notes]
// 1. Delete the char at _readBufPtr
// 2. mybeep() and return false if at _readBufEnd
// 3. Move the remaining string left for one character
// 4. The cursor should stay at the same position
// 5. Remember to update _readBufEnd accordingly.
// 6. Don't leave the tailing character.
// 7. Call "moveBufPtr(...)" if needed.
//
// For example,
//
// cmd> This is the command
//              ^                (^ is the cursor position)
//
// After calling deleteChar()---
//
// cmd> This is he command
//              ^
//
bool
CmdParser::deleteChar()
{
   // TODO...
   if(_readBufPtr == _readBufEnd){
       mybeep();
       //cout << "you are 87!\n";
       return false;
   }
   else{
   char* iter = _readBufPtr;
   while(iter < _readBufEnd - 1){
       *iter = *( iter + 1 );
       iter = iter + 1;
   }
   *iter = ' ';
   *_readBufEnd = ' ';
   _readBufEnd = _readBufEnd - 1;
   return true;
   }
}

// 1. Insert character 'ch' for "repeat" times at _readBufPtr
// 2. Move the remaining string right for "repeat" characters
// 3. The cursor should move right for "repeats" positions afterwards
// 4. Default value for "repeat" is 1. You should assert that (repeat >= 1).
//
// For example,
//
// cmd> This is the command
//              ^                (^ is the cursor position)
//
// After calling insertChar('k', 3) ---
//
// cmd> This is kkkthe command
//                 ^
//
void
CmdParser::insertChar(char ch, int repeat)
{
   // TODO...
   assert(repeat >= 1);
   for(int i = 0; i < repeat; i++){
   char* iter = _readBufEnd;
   while(iter != _readBufPtr ){
       *iter = *( iter - 1 );
       iter = iter - 1;
   }
   *_readBufPtr = ch;
   _readBufEnd = _readBufEnd + 1;
   moveBufPtr(_readBufPtr + 1);
   }
}

// 1. Delete the line that is currently shown on the screen
// 2. Reset _readBufPtr and _readBufEnd to _readBuf
// 3. Make sure *_readBufEnd = 0
//
// For example,
//
// cmd> This is the command
//              ^                (^ is the cursor position)
//
// After calling deleteLine() ---
//
// cmd>
//      ^
//
void
CmdParser::deleteLine()
{
   // TODO...
   for(int i = 0; i<READ_BUF_SIZE; i++){
       *(_readBuf + i)=char(0);
       if ((_readBuf + i) >= _readBufEnd){
           break;
       }
   }
   _readBufPtr = _readBufEnd = _readBuf;
   *_readBufPtr = 0;
}


// This functions moves _historyIdx to index and display _history[index]
// on the screen.
//
// Need to consider:
// If moving up... (i.e. index < _historyIdx)
// 1. If already at top (i.e. _historyIdx == 0), beep and do nothing.
// 2. If at bottom, temporarily record _readBuf to history.
//    (Do not remove spaces, and set _tempCmdStored to "true")
// 3. If index < 0, let index = 0.
//
// If moving down... (i.e. index > _historyIdx)
// 1. If already at bottom, beep and do nothing
// 2. If index >= _history.size(), let index = _history.size() - 1.
//
// Assign _historyIdx to index at the end.
//
// [Note] index should not = _historyIdx
//
void
CmdParser::moveToHistory(int index)
{
   // TODO...
   bool inbottom = _historyIdx == (int)_history.size();
   bool intop = _historyIdx == 0;
   if(0 <= index && index < (int)_history.size()){
         if(index < _historyIdx){//move up
            if(_historyIdx == (int)_history.size() ){
               //if you begin from the newest line and
               //there have not store anything yet
               //push_back the _readBuf!
               _tempCmdStored = true;
               if (_readBufEnd == _readBuf) _history.push_back("");
               else{
               string tmp = "";
               int longs = (_readBufEnd - _readBuf) / sizeof(*_readBufEnd);
               for(int i=0; i<longs; i++){
                    tmp = tmp + *(_readBuf + i);
               }
               //cout << "\n" << ">>> temp line stored: " << tmp << '\n';
               _history.push_back(tmp);
               //cout << "\n" << ">>> after stored: " << _history[_history.size()-1] << '\n';
               }
            }
          }
          int reserve_historyIdx = _historyIdx;
          int reserve_index = index;
          _historyIdx = index;
          retrieveHistory();
          if(reserve_index > reserve_historyIdx){//move down
              if(reserve_index == (int)_history.size() - 1 && _tempCmdStored){
                    _history.pop_back();
                    _tempCmdStored = false;
              }
          }
   }
   else{
        if(inbottom&&(index > _historyIdx))
            mybeep();
        else if(intop&&(index < _historyIdx))
            mybeep();
        if (_history.size()!=0){
            if(0 > index)
                moveToHistory(0);
            else if (!inbottom && index >= (int)_history.size())//if in bottom and press down, just beep!
                if(_tempCmdStored)
                moveToHistory((int)_history.size()-1);
        }
   }
}


// This function adds the string in _readBuf to the _history.
// The size of _history may or may not change. Depending on whether
// there is a temp history string.
//
// 1. Remove ' ' at the beginning and end of _readBuf
// 2. If not a null string, add string to _history.
//    Be sure you are adding to the right entry of _history.
// 3. If it is a null string, don't add anything to _history.
// 4. Make sure to clean up "temp recorded string" (added earlier by up/pgUp,
//    and reset _tempCmdStored to false
// 5. Reset _historyIdx to _history.size() // for future insertion
//
void
CmdParser::addHistory()
{
   // TODO...
   if (_tempCmdStored == true){
       //cout << '\n' << "temp delete: " << _history[_history.size() - 1];
       _history.pop_back();
       _tempCmdStored = false;
   }
   if (_readBuf != _readBufEnd){
       int starts;
       int ends;
       int longs = (_readBufEnd - _readBuf) / sizeof(*_readBufEnd);
       //cout << '\n' << longs;
       bool allspaceflag = true;
       for(int iter = 0; iter < longs; iter++){
            if(*(_readBuf+iter) != ' '){
                starts = iter;
                allspaceflag = false;
                break;
            }
       }
       for(int iter = longs-1; iter >= 0; iter--){
            if(*(_readBuf+iter) != ' '){
                ends = iter;
                allspaceflag = false;
                break;
            }
       }
       //int sizes = ends - starts +1;
       //cout << '\n' << "sizes:" << sizes << ", starts:" << starts << ", ends:" << ends;
       if(!allspaceflag){

           string tmp = "";
           for(int i=starts; i<=ends; i++){
                tmp = tmp + *(_readBuf + i);
           }
           //cout << "\n" << tmp;
           _history.push_back(tmp);
           _historyIdx = _history.size();
       }
    }
}


// 1. Replace current line with _history[_historyIdx] on the screen
// 2. Set _readBufPtr and _readBufEnd to end of line
//
// [Note] Do not change _history.size().
//
void
CmdParser::retrieveHistory()
{
   deleteLine();
   strcpy(_readBuf, _history[_historyIdx].c_str());
   //cout << _readBuf;
   _readBufPtr = _readBufEnd = _readBuf + _history[_historyIdx].size();
}
