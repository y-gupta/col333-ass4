#include <iostream>
#include <vector>
#include <map>
#include <cassert>
#include <cmath>
#include <cstdlib>
#include <string>

using namespace std;

float P_FACECARD = (4.f/13);
float P_NUMCARD = (1.f/13);

class State{
public:

	string why;
	bool canSplit;
	bool final,over;
	
	int score,dealerScore;
	
	float bet;
	float V;char action;

	vector<int>dealerCards;
	vector<int>playerCards;


  State(){
    bet=1;over=false;
    action='N';
    V=2.5*rand()/float(RAND_MAX-1)-1;
    score=dealerScore=0;canSplit=false;final=false;
  }
  
  void init(int p1,int p2,int d){
    playerCards.clear();
    playerCards.push_back(p1);
    playerCards.push_back(p2);
    score=p1+p2;
    dealerCards.clear();
    dealerCards.push_back(d);
    dealerScore=d;
    bet=1;
    over=final=false;
    if(p1==p2)
      canSplit=true;
    else
      canSplit=false;
  }
  
  int hash(){
    int h=0;
    assert(score <= 31);//score cant be more than 31!
    assert(dealerScore <= 31);
    h += score; //5 bits
    bool hasAce = 0;
    // for(auto &c:playerCards)
    //   if(c==1)
    //     hasAce=1;
    // if(hasAce){
    //   h += 32 * hasAce;
    //   h += 128 * playerCards[0];
    // }
    if(canSplit)
    {
      h+= 64 * 1;
      h+= 128 * playerCards[0];//4 bits
    }
    h+= 128*16 * dealerCards[0];//4 bits
    assert(h < 128*16*16);
    h+= 128*256* dealerScore;
    h+= 128*256*32 * (final?1:0);
    return h;
  }
  //state change by actions
  int applyS(){
    final = true;
    why="stood";
    return hash();
  }
  float QsS(map<int,State> &states){ //stand
    State initial = *this;
    int h=applyS();
    auto res=states.find(h);
    if(res == states.end()){
      auto res2 = states.insert(make_pair(h,*this));
      res = res2.first;
    }
    *this = initial;
    return res->second.V;
  }
  int applyH(int card){ // modifies self state deterministically (takes all parameters that can be random) and returns hash of new state.
    assert(playerCards.size() >= 2); //can't hit otherwise!
    canSplit=false; //cant split anymore!

    //todo: handle soft hands
	//11 means we treated ace as 11 and 1 means we treated ace as 1 but we get card as 1 which represents an ace

	if(card==1 && score+11<=21){
    	playerCards.push_back(11);
    	score+=11;
	}
	else{
		playerCards.push_back(card);
    	score+=card;
	}
    if(score>21){
    	for(int i = 0 ; i<playerCards.size() ; i++){
    		if(playerCards[i] == 11){
    			playerCards[i] = 1 ;
				score-=10 ;
    			break;
			}
		}
	}
	if(score>=21){
      final=true;
      why="busted while hitting";
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
  float QsD(map<int,State> &states){ //double
    return -1000;
  }
  float applyP(int card){ // split can occur only when we have 2 cards of same denomination so y do we need a argument here ** no need to use card argument here **
    assert(playerCards.size() == 2); //can't split otherwise!
    assert(playerCards[0]==playerCards[1]);
    
	playerCards[1]=card; // y u setting pCards[1] = card
    
	score = playerCards[0] + playerCards[1];
    
    //todo: handle soft hands 
    /*
	Note a hand is called soft hand if any Ace
	is treated as 11 which would result in bust
	if both cards are same, so no soft hands in
	case of split. 
	P.S. : Please read the assignment specs once plz
	*/
    
    if(playerCards[0]==playerCards[1])
      canSplit=true;
    if(score>=21){ // how can someone bust if they have only 2 cards
      final=true;
      why="busted while splitting";
    }
    return hash();
  }
  float QsP(map<int,State> states){ //sPlit
    State initial = *this;
    int h; float Q=0;
    for(int i=1;i<=10;i++){
      h=applyP(i);
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

  int applyE(int card){ // modifies self state deterministically (takes all parameters that can be random) and returns hash of new state.
    assert(dealerCards.size() >= 1); //can't hit otherwise!
    if(score>21){//player busted
      over=true;
      V=-bet;
    }else if(dealerScore < 17){
      if(card == 1){ //if card is ace then we chose most favourable value
      	if(dealerScore+10<=21){
      		dealerScore+=11 ;
      		dealerCards.push_back(11);
		}
	  }else{
	  	dealerScore += card;
	  	dealerCards.push_back(card);
	  }
    }else{
      //todo: handle soft hands 
      //dealer might have an ace in his hand earlier so we need to reduce the value by 10 and need to draw another card but place 11 or 1 in the card deac to denote what value of ace u considered
	  for(int i = 0 ; i< dealerCards.size() ; i++){
      	 if(dealerCards[i] == 11 && dealerScore>21){
      		dealerCards[i] = 1 ;
      		dealerScore-=10 ;
 			  break;
		 }
	  }
	  if(dealerScore<17){
	  	//TODO: draw an other card
	  }
	  over=true;
      if(dealerScore>21){
        //dealer busted
        V=bet;
      }else if(score == dealerScore){
        V=0;
      }else if(dealerScore > score){
        V=-bet;
      }else if(score == 21){
        V=1.5*bet;
      }else if(score>dealerScore){
        V=bet;
      }
    }
    return hash();
  }

  float QsE(map<int,State> states){ //end the game
    State initial = *this;
    int h; float Q=0,V1=0;
    for(int i=1;i<=10;i++){
      h=applyE(i);
      if(over){
        V1=V;
      }else{
        auto res=states.find(h);
        if(res == states.end()){
          auto res2=states.insert(make_pair(h,*this));
          res=res2.first;
        }
        V1=res->second.V;
      }
      Q+=V1*(i==10?P_FACECARD:P_NUMCARD);
      *this = initial; //reset to original
    }
    return Q;
  }

  float nextV(map<int,State> &states){//calc's next value of V and returns the difference.
    float V1=0;
    if(over){
      V1=0;
      action='N';
    }else if(final){
      V1=QsE(states);
      action='E';
    }else{
      float QH,QD,QP,QS;
      QH=QsH(states);
      QD=QsD(states);
      if(canSplit)
        QP=QsP(states);
      QS=QsS(states);
      float max=QH;char argMax='H';
      if(QD>max)
        max=QP,argMax='D';
      if(canSplit && QP>max)
        max=QP,argMax='P';
      if(QS>max)
        max=QS,argMax='S';
      action=argMax;
      V1=max;
      // cerr<<argMax<<endl;
    }
    float diff = V1-V;
    V=V1;
    return diff;
  }
  void print(){
    cout<<"P(";
    for(auto &p:playerCards)
      cout<<p<<",";
    cout<<") D(";
    for(auto &d:dealerCards)
      cout<<d<<",";
    cout<<")";
    printf(" - %c (%f) %d %s\n",action,V,final,why.c_str());
  }
};

map<int,State> states;
int main(){
  //todo: list all initial states -- i think the things that u have done below is state initialization
  for(int i=1;i<=10;i++){
    for(int j=i;j<=10;j++){
      for(int k=1;k<=10;k++){
        State s;
        s.init(i,j,k);
        states.insert(make_pair(s.hash(),s));
      }
    }
  }

  //todo: do value iteration on all states in map
  cout<<"Num initial states: "<<states.size()<<endl;

  float maxDelta=1000,delta;
  int minN=15;int n=0;

  while(n<minN || maxDelta>0.01){
    cerr<<"maxDelta: "<<maxDelta<<endl;
    maxDelta = 0;
    for(auto &s:states){
      delta=fabs(s.second.nextV(states));
      if(delta>maxDelta)
        maxDelta=delta;
    }
    n++;
  }

  cout<<"Num total states: "<<states.size()<<endl;

  for(auto &s:states){
    s.second.print();
  }
	return 0 ;
}
