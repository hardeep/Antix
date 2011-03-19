#include "DrawServer.h"
#include "Home.h"

#include <math.h>
#include <stdlib.h>
#include <unistd.h>
#include <stdio.h>

using namespace Network;

DrawServer* DrawServer::m_instance = NULL;

DrawServer::DrawServer()
{    
    this->m_windowSize = 600;
    this->m_worldSize = 1.0;
    this->m_FOVEnabled = false;
    this->m_FOVAngle = 0.0;
    this->m_FOVRange = 0.0;
    this->m_homeRadius = 20.0;
    this->m_framestep = 0;
}

DrawServer::~DrawServer() 
{
}

DrawServer* DrawServer::getInstance()
{
    if (!m_instance)
        m_instance = new DrawServer();

    return m_instance;
}

int DrawServer::init(int argc, char** argv)
{
	if (Server::init(argv[1]) < 0)
		return -1;

    if (argc > 2)
    {
        //<window_size> <world_size> <home_radius> <enable_FOV> [ <FOV_angle> <FOV_range> ]
        this->m_windowSize = atoi(argv[2]);
        this->m_worldSize = strtof(argv[3], NULL);
        this->m_homeRadius = strtof(argv[4], NULL);
        this->m_FOVEnabled = atoi(argv[5]);
        if (this->m_FOVEnabled)
        {
            this->m_FOVAngle = strtof(argv[6], NULL);
            this->m_FOVRange = strtof(argv[7], NULL);
        }
    }
    
    return 0;
}

void DrawServer::initTeams()
{
    /*
    while ((teamData = this->m_redisCli->blpop()) != NULL)
    {
        int teamID, colR, colG, colB;
        float homeX, homeY;

        sscanf(teamData->message.c_str(), TEAM_PATTERN, &teamID, &homeX, &homeY, &colR, &colG, &colB);
        Math::Position *homePos = new Math::Position(homeX, homeY, 0.0);
        Game::Home * home = new Game::Home(homePos);
        
        //figure something out for the colour
        
        this->m_teams[teamID] = new Game::Team();
        this->m_teams[teamID]->m_Home = home;
    }
     * */
    
    if (this->m_teams.size() == 0)
    {
    	Math::Position *homePos = new Math::Position(55.0, 150.0, 0.0);
        Game::Home * home = new Game::Home(homePos);        
        this->m_teams[1] = new Game::Team();
        this->m_teams[1]->m_Home = home;
    }

    printf("Teams=%d\n", this->m_teams.size());
}

void DrawServer::updateObject(Msg_RobotInfo newInfo)
{
	//if (newInfo == NULL) return;
	
    Math::Position *pos = new Math::Position(
    	(float)newInfo.x_pos, (float)newInfo.x_pos, (float)newInfo.angle);
    if (!pos)
    { 
    	DEBUGPRINT("Object %d: Invalid position\n", newInfo.newInfo.id);
    	return;
    }
    
    if(newInfo.id >> TEAM_ID_SHIFT == 0)
    {
        if (!this->m_pucks[newInfo.id])
        {
        	  this->m_pucks[newInfo.id] = new Game::Puck(pos);
        }
      	else
      	{
      		  this->m_pucks[newInfo.id]->getPosition()->setX(newInfo.x_pos);
      		  this->m_pucks[newInfo.id]->getPosition()->setY(newInfo.y_pos);        		
      	}
    }
    else
    {
      	if (!this->m_robots[newInfo.id])
      	{
        		if (!this->m_teams[1])//newInfo.id/TEAM_ID_SHIFT])    
        		{
        			  DEBUGPRINT("No home for team %d\n", 1);//newInfo.id/TEAM_ID_SHIFT);
        			  return;
        		}         			
  			    this->m_robots[newInfo.id] = new Game::Robot(pos, this->m_teams[1]->getHome());//newInfo.id/TEAM_ID_SHIFT]->getHome());
      	}          	
      	else
      	{
        		this->m_robots[newInfo.id]->getPosition()->setX(newInfo.x_pos);
        		this->m_robots[newInfo.id]->getPosition()->setY(newInfo.y_pos);        		
      	}
        
        //this->m_robots[newInfo.id]->m_PuckHeld = this->m_pucks[newInfo.puck_id];
    }
    
    //check for overflow
    this->m_framestep++;
}


//SERVER METHODS

int DrawServer::handler(int fd)
{
    DEBUGPRINT("Handling file descriptor: %i\n", fd);

    //Get our TCPConnection for this socket.    
    TcpConnection * l_Conn = this->m_Clients[fd];	
	if (l_Conn == NULL)
    {
		return -1; // no such socket descriptor
	}	
	
	//Get message header
	uint16_t l_sender=-1, l_senderMsg =-1;	
	NetworkCommon::requestHeader(l_sender, l_senderMsg, l_Conn);
	if (l_sender == -1 || l_senderMsg == -1) return -1;
		
    switch(l_sender)
    {
        case(SENDER_GRIDSERVER):
            switch(l_senderMsg)
            {
                case(MSG_GRIDDATAFULL) :
                {
                    //Receive the total number of robots we are getting sens info for.
                    Msg_MsgSize l_NumObjects;
					NetworkCommon::requestMessageSize(l_NumObjects, l_Conn);
                    DEBUGPRINT("Expecting %d objects.\n", l_NumObjects.msgSize );

                    //Go through all of the objects we expect position data for.
                    for (int i = 0; i < l_NumObjects.msgSize;i++)
                    {
                        //First get the header telling us how many objects are sensed and for which object.
                        Msg_RobotInfo l_ObjInfo;
                        unsigned char l_ObjInfoBuf[l_ObjInfo.size];
                        
                        recvWrapper(l_Conn, l_ObjInfoBuf, l_ObjInfo.size);
                        unpack(l_ObjInfoBuf, Msg_RobotInfo_format,
                                &l_ObjInfo.id, &l_ObjInfo.x_pos, &l_ObjInfo.y_pos, &l_ObjInfo.speed, &l_ObjInfo.angle, &l_ObjInfo.puck_id );

                        DEBUGPRINT("Object: newInfo.id=%d\tx=%d\ty=%d\tspeed=%d\tangle=%d\tpuck=%d\n",
                                l_ObjInfo.id, l_ObjInfo.x_pos, l_ObjInfo.y_pos, l_ObjInfo.speed, l_ObjInfo.angle, l_ObjInfo.puck_id );
                                
                        updateObject(l_ObjInfo);
                    }

                }
                
                case(MSG_GRIDDATACOMPRESS) : 
                {
                }
                
                //case team initialization info
            }
            break;
    }
    
    return 0;
}

int DrawServer::handleNewConnection(int fd)
{
    return 0;
}

int DrawServer::allConnectionReadyHandler()
{
    return 0;
}






// NETWORK PACKING METHODS


int DrawServer::sendWrapper(TcpConnection * conn, unsigned char* buffer, int msgSize)
{
    if (conn->send(buffer, msgSize) == -1)
    {
        DEBUGPRINT("Failed to send a message.\n");
        return -1;
    }
    return 0;
}

int DrawServer::recvWrapper(TcpConnection* conn, unsigned char* buffer, int msgSize)
{
    if (conn->recv(buffer, msgSize) == -1)
    {
        DEBUGPRINT("Failed to receive a message.\n");
        return -1;
    }
    return 0;
}

int DrawServer::packHeaderMessage(unsigned char* buffer, uint16_t sender, uint16_t message)
{
    Msg_header l_Header;

    l_Header.sender = sender;
    l_Header.message = message;

    if (pack(buffer, Msg_header_format, l_Header.sender, l_Header.message) != l_Header.size)
    {
        DEBUGPRINT("Failed to pack a header message. Sender: %d, Message: %d", sender, message);
        return -1;
    }
    return 0;
}


