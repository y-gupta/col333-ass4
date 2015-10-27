#include <iostream>
#include <vector>
#include <map>
#include <cassert>
#include <cmath>
#include <cstdlib>
#include <string>
#include <fstream>
using namespace std;

double P_FACECARD;
double P_NUMCARD;

class State{
public:
	bool canSplit,canDouble,playerHard,playerSoft,dealerSoft,dealerHard,dealerCanBlackjack;
	bool isFinal,isOver;
	int playerScore,dealerScore;
  double QH,QD,QP,QS,QE;

	double V;char action;

  State(){
    QH=QD=QP=QS=QE=0;
    dealerCanBlackjack=false;
    isOver=false;
    action='N';canDouble=false;
    playerSoft=playerHard=false;
    dealerSoft=dealerHard=false;
    playerScore=dealerScore=0;canSplit=false;isFinal=false;
    initV();
  }
  void initV(){
    V=4*rand()/double(RAND_MAX-1)-2;
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
    isOver=isFinal=false;
    canDouble = true;
    canSplit = (p1==p2);
    if(p1==1||p2==1)
      playerHard=true;
    handleSoft(0);
    dealerHard=false;
    if(d==1){
      dealerSoft=true;
      dealerScore=11;
    }
    if(dealerScore==11||dealerScore==10)
      dealerCanBlackjack=true;
    else
      dealerCanBlackjack=false;
  }

  int hash(){
    unsigned int h=0;
    assert(playerScore <= 30);//playerScore cant be more than 30!
    assert(dealerScore <= 30);
    h += (1<<0) * playerScore; //5 bits
    h += (1<<5) * dealerScore;//5 bits
    h += (1<<10) * playerSoft;
    h += (1<<11) * playerHard;
    h += (1<<12) * canSplit;
    h += (1<<13) * isFinal;
    h += (1<<14) * canDouble;
    h += (1<<15) * dealerHard;
    h += (1<<16) * dealerSoft;
    h += (1<<17) * isOver;
    h += (1<<18) * dealerCanBlackjack;
    // h += (1<<18) * int(bet);
    return (int)h;
  }
  void handleSoft(int card){
    if(!playerSoft && card==1)
      playerHard=true;
    if(playerHard && playerScore+10<=21){
      assert(!playerSoft);
      playerScore += 10;
      playerSoft=true;
      playerHard=false;
    }
    if(playerSoft && playerScore > 21){
      assert(!playerHard);
      playerScore -= 10;
      playerSoft=false;
      playerHard=true;
    }
  }
  void handleDealerSoft(int card){
    if(!dealerSoft && card==1)
      dealerHard=true;
    if(dealerHard && dealerScore+10<=21){
      assert(!dealerSoft);
      dealerScore += 10;
      dealerSoft=true;
      dealerHard=false;
    }
    if(dealerSoft && dealerScore > 21){
      assert(!dealerHard);
      dealerScore -= 10;
      dealerSoft=false;
      dealerHard=true;
    }
  }
  //state change by actions
  int applyS(){
    assert(!isFinal);
    isFinal = true;
    canSplit = false;
    return hash();
  }
  int applyH(int card){ // modifies self state deterministically (takes all parameters that can be random) and returns hash of new state.
    assert(!isFinal);
    canSplit=false; //cant split anymore!
    canDouble=false;

    playerScore+=card;
    handleSoft(card);
	  if(playerScore>=21){
      isFinal=true;
    }
    return hash();
  }
  int applyD(int card){
    assert(canDouble);
    canSplit=false; //cant split anymore!
    canDouble=false;
    isFinal=true;
    // bet *= 2;
    playerScore+=card;
    handleSoft(card);

    return hash();
  }
  int applyP(int card){
    assert(canSplit);
    assert(!isFinal);
    canSplit=false;
    assert(!playerHard);
    if(playerSoft){
      isFinal=true;
      canDouble=false;
      playerScore = 11 + card;
    }else{
      assert(canDouble);
      assert(playerScore%2==0);
      if(playerScore == card*2){
        canSplit=true;
      }else{
        playerScore=playerScore/2 + card;
        handleSoft(card);
      }
    }
    if(playerScore==21){
      isFinal=true;
    }
    assert(playerScore<=21);
    // bet+=1;
    return hash();
  }
  int applyE(int card){ // modifies self state deterministically (takes all parameters that can be random) and returns hash of new state.
    if(playerScore>21){//player busted
      isOver=true;
      V=-100;
    }else if(dealerScore < 17){
      dealerScore += card;
      handleDealerSoft(card);
      if(dealerScore!=21)
        dealerCanBlackjack=false;
    }else{
      isOver=true;
      if(dealerScore>21 || playerScore > dealerScore){
        V=100;
      }else if(dealerScore > playerScore){
        V=-100;
      }else if(playerScore == 21){
        if(dealerCanBlackjack)
          V=canDouble?0:-100;
        else
          V=canDouble?150:0;
      }else{
        V=0;
      }
    }
    return hash();
  }
  double QsS(map<int,State> &states){ //stand
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
  double QsH(map<int,State> &states){ //hit, returns Q(s,H)
    State initial = *this;
    int h; double Q=0;
    for(int i=1;i<=10;i++){
      h=applyH(i);
      auto res=states.find(h);
      if(res == states.end()){
        auto res2=states.insert(make_pair(h,*this));
        res=res2.first;
      }
      *this = initial; //reset to original
      Q+=res->second.V*(i==10?P_FACECARD:P_NUMCARD);
    }
    return Q;
  }

  double QsD(map<int,State> &states){ //double
    State initial = *this;
    int h; double Q=0;
    for(int i=1;i<=10;i++){
      h=applyD(i);
      auto res=states.find(h);
      if(res == states.end()){
        auto res2=states.insert(make_pair(h,*this));
        res=res2.first;
      }
      *this = initial; //reset to original
      Q+=res->second.V*(i==10?P_FACECARD:P_NUMCARD);
    }
    return Q*2;
  }

  double QsP(map<int,State> &states){ //sPlit
    State initial = *this;
    int h; double Q=0;
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

  double QsE(map<int,State> &states){ //end the game
    State initial = *this;
    int h; double Q=0;
    for(int i=1;i<=10;i++){
      h=applyE(i);
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

  double nextV(map<int,State> &states){//calc's next value of V and returns the difference.
    double V1=0;
    if(isOver){
      action='N';
      return 0;
    }else if(isFinal){
      QE=QsE(states);
      QD=QH=QS=QP=-1;
      V1=QE;
      action='E';
    }else{

      QH=QsH(states);
      QS=QsS(states);

      if(canDouble)
        QD=QsD(states);
      else
        QD=-1;

      if(canSplit)
        QP=QsP(states);
      else
        QP=-1;

      double max=QH;char argMax='H';

      if(canSplit && QP>max)
        max=QP,argMax='P';
      if(canDouble && QD>max)
        max=QD,argMax='D';
      if(QS>max)
        max=QS,argMax='S';

      action=argMax;
      V1=max;
      // cerr<<argMax<<endl;
    }
    double diff = V1-V;
    V=V1;
    return diff;
  }
  void print()
  {
    printf("P(%d,A%d,P%d,D%d) D(%d,%d) (S%.2f,H%.2f,D%.2f,P%.2f,E%.2f) - %c\n",
      playerScore,playerSoft*2+playerHard,canSplit,canDouble,
      dealerScore,dealerSoft*2+dealerHard,
      QS,QH,QD,QP,QE,action
    );
  }
};

int main(int argc,char **argv)
{
  map<int,State> states;

  if(argc>1)
    sscanf(argv[1],"%lf",&P_FACECARD);
  else
    P_FACECARD = 0.307692;//(4.f/13);
  P_NUMCARD = (1-P_FACECARD)/9;
  cout<<P_FACECARD<<","<<P_NUMCARD<<endl;

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

  double maxDelta=1000,delta;
  int minN=15;int n=0;

  while(n<minN || maxDelta>1e-18){
    maxDelta = 0;
    for(auto &s:states){
      delta=fabs(s.second.nextV(states));
      if(delta>maxDelta)
        maxDelta=delta;
    }
    cout<<"maxDelta: "<<maxDelta<<endl;
    n++;
  }

  cout<<"Num total states: "<<states.size()<<endl;

  // for(auto &s:states){
  //   s.second.print();
  // }
  ofstream ft("Policy.txt");
  for(int h=5;h<=19;h++){
    int i=h/2;
    int j=h-i;
    if(i==j){
      i-=1;
      j+=1;
    }
    ft<<h<<"\t";
    for(int k=2;k<=11;k++){
      State s;
      s.init(i,j,k);
      auto ss=states.find(s.hash());
      assert(ss != states.end());
      ft<<ss->second.action<<(k==11?"":" ");
    }
    ft<<endl;
  }
  for(int i=2;i<=9;i++){
    ft<<"A"<<i<<"\t";
    for(int k=2;k<=11;k++){
      State s;
      s.init(1,i,k);
      auto ss=states.find(s.hash());
      assert(ss != states.end());
      ft<<ss->second.action<<(k==11?"":" ");
    }
    ft<<endl;
  }
  for(int i=2;i<=11;i++){
    if(i==11)
      ft<<"AA\t";
    else
      ft<<i<<i<<"\t";
    for(int k=2;k<=11;k++){
      State s;
      s.init(i,i,k);
      auto ss=states.find(s.hash());
      assert(ss != states.end());
      ft<<ss->second.action<<(k==11?"":" ");
    }
    if(i!=11)
      ft<<endl;
  }
	return 0;
}
