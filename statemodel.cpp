#include<iostream>
#include<vector>
#include<map>

using namespace std ;

class State{
	bool canSlit;
	bool final;
	int value ;
	vector<int>dealerCards;
	vector<int>playerCards;
};

//state change by actions
void S(State s){ //stand
	return ;
}
void H(State s){ //hit
	return ;
}

void D(State s){ //double
 	return ;
}

void P(state s){ //sPlit
	return ;
}

int main(){
	return 0 ;
}
