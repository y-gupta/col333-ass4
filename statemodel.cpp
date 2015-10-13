#include <iostream>
#include <vector>
#include <map>
#include <cassert>

using namespace std;
float P_FACECARD = (4.f/13);
float P_NUMCARD = (1.f/13);
class State{
	bool canSplit;
	bool final;
	int value;
  float V;char action;
	vector<int>dealerCards;
	vector<int>playerCards;
  State(){
    V=0;value=0;canSplit=false;final=false;
  }
  int hash(){
    int h=0;
    assert(value <= 31);//value cant be more than 31!
    h += value; //5 bits
    bool hasAce = 0;
    for(auto &c:playerCards)
      if(c==1)
        hasAce=1
    if(hasAce){
      h += 32 * hasAce;
      h += 128 * playerCards[0];
    }
    if(canSplit)
    {
      h+= 64 * 1;
      h+= 128 * playerCards[0];//4 bits
    }
    h+= 128*16 * dealerCards[0];//4 bits
    assert(h < 128*32);
    return h;
  }
  //state change by actions
  int applyS(){
    final = true;
    return hash();
  }
  float QsS(map<int,State> &states){ //stand
    h=applyS();
    auto res=states.find(h);
    if(res == states.end()){
      auto res2 = states.insert(make_pair(h,*this));
      res = res2.first;
    }
    return res->second.V;
  }
  int applyH(int card){ // modifies self state deterministically (takes all parameters that can be random) and returns hash of new state.
    assert(playerCards.size() >= 2); //can't hit otherwise!
    playerCards.push_back(card);
    score += card;
    //todo: handle soft hands
    if(score>21){
      //todo: busted
      final=true;
    }
    return hash();
  }
  float QsH(map<int,State> &states){ //hit, returns Q(s,H)
    State initial = *this;
    int h; float Q=0;
    for(int i=1;i<=10;i++){
      h=applyH(i);
      auto res=states.find(h);
      if(res == states.end()){
        auto res2=states.insert(make_pair(h,*this));
        res=res2.first;
      }
      Q+=res->second.V*(i==10?P_FACECARD:P_NUMCARD);
      *this = initial; //reset to original
    }
    return Q;
  }
  float QsD(){ //double
    return 0;
  }
  float applyP(int card){
    assert(playerCards.size() == 2); //can't split otherwise!
    assert(playerCards[0]==playerCards[1]);
    playerCards[1]=card;
    score = playerCards[0] + playerCards[1];
    //todo: handle soft hands
    if(score>21){
      //todo: busted
      final=true;
    }
    return hash();
  }
  float QsP(){ //sPlit
    return 0;
  }

  float nextV(map<int,State> &states){//calc's next value of V and returns the difference.
    float V1=0;
    if(final){
      //TODO: do dealer hits and end game
    }else{
      float QH,QD,QP,QS;
      QH=QsH(states);
      QD=QsD(states);
      QP=QsP(states);
      QS=QsS(states);
      float max=QH;char argMax='H';
      if(QD>max)
        max=QP,argMax='D';
      if(QP>max)
        max=QP,argMax='P';
      if(QS>max)
        max=QS,argMax='S';
      action=argMax;
      V1=max;
    }
    float diff = V1-V;
    V=V1;
    return diff;
  }
};

int main(){
  map<int,State> states;
  //todo: list all initial states
  //todo: do value iteration on all states in map
	return 0 ;
}
