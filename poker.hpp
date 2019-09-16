#pragma once
namespace NSP_POKER
{

enum class enumPokerColor
{
    DIAMOND = 0,
    CLUB    = 1,
    HEART   = 2,
    SPADE   = 3,
    UNKNOWN = 4
};

class CPokerCard
{
    
public:
    CPokerCard();
    ~CPokerCard();
    CPokerCard(int number, enumPokerColor color);
    CPokerCard(int number);

public:
    bool set_poker_card(int number, enumPokerColor color);

    int get_number(){ return m_nNumber; }

    enumPokerColor get_color(){ return m_enumColor; }

    bool operator < (CPokerCard& pokerCard);

private:
    int                 m_nNumber;

    int                 m_nNumberWeight;

    enumPokerColor      m_enumColor;
};

class CPokerComp
{
public:
    CPokerComp();
    virtual ~CPokerComp() = 0;
};

//单张牌
class CSinglePoker: public CPokerComp
{
public:
    CSinglePoker();
    ~CSinglePoker();

    bool operator < (CSinglePoker& poker);

private:
};

}

