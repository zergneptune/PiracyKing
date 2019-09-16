#include "poker.hpp"
#include "utility.hpp"
using namespace NSP_POKER;

CPokerCard::CPokerCard(){}
CPokerCard::~CPokerCard(){}

CPokerCard::CPokerCard(int number, enumPokerColor color): m_nNumber(number), m_enumColor(color){}

CPokerCard::CPokerCard(int number): m_nNumber(number), m_enumColor(enumPokerColor::UNKNOWN){}

bool CPokerCard::set_poker_card(int number, enumPokerColor color)
{
    if(number < 0 || number > 15)
    {
        return false;
    }

    m_nNumber = number;
    m_enumColor = color;
    return true;
}

bool CPokerCard::operator < (CPokerCard& pokerCard)
{
    int number = pokerCard.get_number();
    enumPokerColor color = pokerCard.get_color();
    return (m_nNumber != number ? m_nNumber < number : m_enumColor < color);
}

/*
***********************
** POKER GAME！！！
***********************
*/
CPokerComp::CPokerComp(){}
CPokerComp::~CPokerComp(){}

//单张牌
CSinglePoker::CSinglePoker(){}
CSinglePoker::~CSinglePoker(){}

bool CSinglePoker::operator < (CSinglePoker& poker)
{
    return false;
}