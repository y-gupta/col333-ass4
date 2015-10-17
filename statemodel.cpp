#include <iostream>
#include <vector>
#include <map>
#include <cassert>
#include <cmath>
#include <cstdlib>
#include <string>

using namespace std;

float P_FACECARD;
float P_NUMCARD;

class State{
public:
	bool canSplit,canDouble,playerHard,playerSoft,dealerSoft,dealerHard,dealerCanBlackjack;
	bool isFinal,over;
	int playerScore,dealerScore;

	float bet;
	float V;char action;

  State(){
    dealerCanBlackjack=false;
    bet=1;over=false;
    action='N';canDouble=false;
    V=2.5*rand()/float(RAND_MAX-1)-1;
    playerSoft=playerHard=false;
    dealerSoft=dealerHard=false;
    playerScore=dealerScore=0;canSplit=false;isFinal=false;
  }

  void init(int p1,int p2,int d){
    if(d==11)
      d=1;
    if(p1==11)
      p1=1;
    if(p2==11)
      p2=1;
    playerScore=p1+p2;
    dealerScore=d;
    bet=1;
    over=isFinal=false;
    canDouble=true;
    canSplit= (p1==p2);
    if(p1==1||p2==1)
      playerHard=true;
    handleSoft(0);
    if(d==1)
      dealerHard=true;
    if(d==1||d==10)
      dealerCanBlackjack=true;
  }

  int hash(){
    unsigned int h=0;
    assert(playerScore <= 31);//playerScore cant be more than 31!
    assert(dealerScore <= 31);
    h += (1<<0) * playerScore; //5 bits
    h += (1<<5)* dealerScore;//5 bits
    h += (1<<10) * playerSoft;
    h += (1<<11) * playerHard;
    h += (1<<12) * canSplit;
    h += (1<<13) * isFinal;
    h += (1<<14) * canDouble;
    h += (1<<15) * dealerHard;
    h += (1<<16) * dealerSoft;
    h += (1<<17) * dealerCanBlackjack;
    h += (1<<18) * int(bet);
    return (int)h;
  }
  //state change by actions
  int applyS(){
    isFinal = true;
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
  void handleSoft(int card){
    if(!playerSoft && card==1)
      playerHard=true;
    if(playerHard && playerScore+10<=21){
      playerScore += 10;
      playerSoft=true;
      playerHard=false;
    }
    if(playerSoft && playerScore > 21){
      playerScore -= 10;
      playerSoft=false;
      playerHard=true;
    }
  }
  int applyH(int card){ // modifies self state deterministically (takes all parameters that can be random) and returns hash of new state.
    canSplit=false; //cant split anymore!
    canDouble=false;

    playerScore+=card;
    handleSoft(card);
	  if(playerScore>=21){
      isFinal=true;
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
  int applyD(int card){
    canSplit=false; //cant split anymore!
    canDouble=false;
    isFinal=true;
    // bet *= 2;
    playerScore+=card;
    handleSoft(card);
    return hash();
  }
  float QsD(map<int,State> &states){ //double
    State initial = *this;
    int h; float Q=0;
    for(int i=1;i<=10;i++){
      h=applyD(i);
      auto res=states.find(h);
      if(res == states.end()){
        auto res2=states.insert(make_pair(h,*this));
        res=res2.first;
      }
      Q+=res->second.V*(i==10?P_FACECARD:P_NUMCARD);
      *this = initial; //reset to original
    }
    return Q*2;
  }
  float applyP(int card){
    assert(canSplit);
    canSplit=false;
    if(playerSoft){
      isFinal=true;
      canDouble=false;
      canSplit=false;
    }else{
      canDouble=true;
      if(playerScore == card*2)
        canSplit=true;
    }

    if(!playerSoft)
      playerScore=playerScore/2 + card;
  	else
      playerScore = 11 + card;

    handleSoft(card);

    if(playerScore>=21){
      isFinal=true;
    }
    return hash();
  }
  float QsP(map<int,State> &states){ //sPlit
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
    return Q*2;
  }

  int applyE(int card){ // modifies self state deterministically (takes all parameters that can be random) and returns hash of new state.
    if(playerScore>21){//player busted
      over=true;
      V=-bet;
    }else if(dealerScore < 17){
	  	dealerScore += card;

      if(!dealerSoft && card==1)
        dealerHard=1;
      if(dealerHard && dealerScore+10<=21){
        dealerScore += 10;
        dealerSoft=true;
        dealerHard=false;
      }
      if(dealerSoft && dealerScore > 21){
        dealerScore -= 10;
        dealerSoft=false;
        dealerHard=true;
      }
      if(dealerCanBlackjack && dealerScore!=21)
        dealerCanBlackjack=false;
    }else{
	    over=true;
      if(dealerScore>21){
        V=bet;
      }else if(dealerScore > playerScore){
        V=-bet;
      }else if(playerScore > dealerScore){
        V=bet;
      }else if(playerScore == 21){
        if(canDouble && !dealerCanBlackjack)
          V=1.5*bet;
        else if(canDouble && dealerCanBlackjack)
          V=0;
        else
          V=-bet;
      }else{
        V=0;
      }
    }
    return hash();
  }

  float QsE(map<int,State> &states){ //end the game
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
    }else if(isFinal){
      V1=QsE(states);
      action='E';
    }else{
      float QH,QD,QP,QS;
      QH=QsH(states);
      QS=QsS(states);
      if(canDouble)
        QD=QsD(states);
      if(canSplit)
        QP=QsP(states);
      float max=QH;char argMax='H';
      if(canSplit && QP>=max)
        max=QP,argMax='P';
      if(canDouble && QD>=max)
        max=QD,argMax='D';
      if(QS>=max)
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
    printf("P(%d,%d,%d,%d) D(%d,%d) %f - %c\n",
      playerScore,playerSoft*2+playerHard,canSplit,canDouble,
      dealerScore,dealerSoft*2+dealerHard,
      V,action
    );
  }
};

int main(int argc,char **argv){
  map<int,State> states;

  if(argc>1)
    sscanf(argv[1],"%f",&P_FACECARD);
  else
    P_FACECARD = 0.307;//(4.f/13);
  P_NUMCARD = (1-P_FACECARD)/9;

  for(int i=1;i<=10;i++){
    for(int j=i;j<=10;j++){
      for(int k=1;k<=10;k++){
        State s;
        s.init(i,j,k);
        states.insert(make_pair(s.hash(),s));
      }
    }
  }

  cout<<"Num initial states: "<<states.size()<<endl;

  float maxDelta=1000,delta;
  int minN=20;int n=0;

  while(n<minN || maxDelta>0.0001){
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
  for(int h=5;h<=19;h++){
    int i=h/2;
    int j=h-i;
    if(i==j){
      i-=1;
      j+=1;
    }
    cerr<<h<<"\t";
    for(int k=2;k<=11;k++){
      State s;
      s.init(i,j,k);
      auto ss=states.find(s.hash());
      assert(ss != states.end());
      cerr<<ss->second.action<<" ";
    }
    cerr<<endl;
  }
  for(int i=2;i<=9;i++){
    cerr<<"A"<<i<<"\t";
    for(int k=2;k<=11;k++){
      State s;
      s.init(1,i,k);
      auto ss=states.find(s.hash());
      assert(ss != states.end());
      cerr<<ss->second.action<<" ";
    }
    cerr<<endl;
  }
  for(int i=2;i<=11;i++){
    if(i==11)
      cerr<<"AA\t";
    else
      cerr<<i<<i<<"\t";
    for(int k=2;k<=11;k++){
      State s;
      s.init(i,i,k);
      auto ss=states.find(s.hash());
      assert(ss != states.end());
      cerr<<ss->second.action<<" ";
    }
    if(i!=11)
      cerr<<endl;
  }
	return 0;
}
