#include "GridServer.h"
#include "Config.h"
#include "Types.h"
#include <map>
#include <vector>
#include "networkCommon.h"

using namespace Network;

GridServer::GridServer():Server()
{
    m_uId = -1;
    m_hb_rcvd = 0;
}
int GridServer::handleNewConnection(int fd)
{
    return 0;
}

int GridServer::allConnectionReadyHandler()
{
    return 0;
}


int GridServer::handler(int fd)
{
	TcpConnection * l_curConnection = m_Clients[fd];
	
	if (l_curConnection == NULL)
    {
		return -1; // no such socket descriptor
	}
	
	uint16_t l_sender=-1, l_senderMsg =-1;
	
	NetworkCommon::requestHeader(l_sender, l_senderMsg, l_curConnection);

	DEBUGPRINT("%u, %u\n", l_sender, l_senderMsg);
	
	if (l_sender == -1 || l_senderMsg == -1) return -1; // bad messages
	
	switch (l_sender)
	{
        case SENDER_CLIENT:
			
            switch(l_senderMsg)
            {
				case (MSG_REQUESTSENSORDATA):
				{
					
					Msg_MsgSize l_msgSize;
					NetworkCommon::requestMessageSize(l_msgSize, l_curConnection);
					int l_numRobots = l_msgSize.msgSize;
					
					if (l_numRobots <= 0)
					{
						DEBUGPRINT("Requested sensor data for <=0 Robots. Game error.\n");
						return -1;
					}
					
					DEBUGPRINT("Client has requested actions for %i robots\n", l_numRobots);
					
					std::vector<int> l_robotIdVector;
					l_robotIdVector.clear();
					
					Msg_RobotId l_robotId;
					unsigned char l_robotIdBuff[l_robotId.size * l_numRobots];
					
					for (int i = 0; i < l_numRobots; i++)
					{
						if (l_curConnection->recv(l_robotIdBuff+(l_robotId.size * i), l_robotId.size) == -1)
						{
							DEBUGPRINT("Could not get robot id for returnSensorData\n");
							return 0;
						}
						unsigned int id;
						unpack(l_robotIdBuff+(l_robotId.size * i),Msg_RobotId_format, &id);
						l_robotIdVector.push_back(id);
						DEBUGPRINT("%i\n", id);
					}

					int l_robotsTotal = l_robotIdVector.size(); // how many robots have sensory data
					int l_totalSensed = 0; // total of all the robots sensory data *how many sensed items in total*
					
					std::map<int, std::vector<sensed_item> > sensed_items_map; // has map that contains a list for each robot in request list
					/*
					 ****************************************************************************************
					 * gridGameInstance.returnSensorData(robot_ids, &sensed_items_map, l_totalSensed); // collect the sensory information
					 ****************************************************************************************
					*/
					
					sensed_item s1 ;
					
					l_totalSensed = 15;
					std::vector<sensed_item> a1;
					
					for (int i =0; i< l_totalSensed; i++)
					{
						s1.robotid = i;
						s1.x = i;
						s1.y = i;
						a1.push_back(s1);
					}
					DEBUGPRINT("got all sensory data\n");
					for (int i = 0; i < l_robotsTotal; i++)
					{
						sensed_items_map[i] = a1;
					}
					DEBUGPRINT("got all robot <= sensory data total of %d robots with %d sensory objects\n", a1.size(),sensed_items_map.size());
					
					Msg_header l_header = {SENDER_GRIDSERVER, MSG_RESPONDSENSORDATA}; // header for response
					memset(&l_msgSize, 0 , l_msgSize.size);
					
					l_msgSize.msgSize = l_robotsTotal;
					
					Msg_SensedObjectGroupHeader l_robotHeader; // for each robot
					Msg_SensedObjectGroupItem l_sensedObject; // for each sensed item
					
					unsigned int l_responseMsgSize = 0;
					
					l_responseMsgSize += l_header.size; // append header size
					l_responseMsgSize += l_msgSize.size; // append size of msg length 
					
					l_responseMsgSize += (l_robotsTotal * l_robotHeader.size); // append the total size for number of robot ids
					l_responseMsgSize += (l_totalSensed * l_robotsTotal * l_sensedObject.size); // all sensed items
					
					printf("msg size: %i\n", l_responseMsgSize);
					
					unsigned char msgBuffer[l_responseMsgSize];
					
					if (pack(msgBuffer, Msg_header_format, l_header.sender, l_header.message) != l_header.size)
					{
						DEBUGPRINT("Could not pack header\n");
						return -1;
					}
					
					int l_position = l_header.size;
					
					if (pack(msgBuffer+l_position, Msg_MsgSize_format, l_msgSize.msgSize) != l_msgSize.size) // pack number of robots
					{
						DEBUGPRINT("Could not pack number of robots\n");
						return -1;
					}
					
					l_position += l_msgSize.size; // shift by robot size header
					
					DEBUGPRINT("AFTER PACKING SIZE %d\n", l_position);
					
					std::map<int, std::vector<sensed_item> >::iterator map_end = sensed_items_map.end(); 
					 // sensed item iterator
					
					DEBUGPRINT("Set up iterator\n");
					
									
					for (std::map<int, std::vector<sensed_item> >::iterator it = sensed_items_map.begin(); it != map_end; it++)
					{
						// for each robot requested
						l_robotHeader.id = it->first;
						l_robotHeader.objectCount = it->second.size();
						DEBUGPRINT("PACKING robot %d with %d sensory objects\n", l_robotHeader.id, l_robotHeader.objectCount);
						if (pack(msgBuffer+l_position, Msg_SensedObjectGroupHeader_format, l_robotHeader.id, l_robotHeader.objectCount) != l_robotHeader.size)
						{
							DEBUGPRINT("Could not pack robot header\n");
							return -1;
						}
						
						l_position += l_robotHeader.size;
						DEBUGPRINT("CURRENT POSITION SIZE %d\n", l_position);
						
						std::vector<sensed_item>::iterator sensedItemIt = it->second.end(); 
						
						for (std::vector<sensed_item>::iterator vecIt = it->second.begin(); vecIt != sensedItemIt; vecIt++)
						{
							// for each object seen by such robot
							l_sensedObject.y = vecIt->y;
							l_sensedObject.x = vecIt->x;
							l_sensedObject.robotid = vecIt->robotid;
							DEBUGPRINT("Packing sensed object id=%d x=%d y=%d | of buffer %d\n", l_sensedObject.robotid, l_sensedObject.x, l_sensedObject.y, l_position);
							if (pack(msgBuffer+l_position, Msg_SensedObjectGroupItem_format, l_sensedObject.robotid, l_sensedObject.x, l_sensedObject.y) != l_sensedObject.size)
							{
								DEBUGPRINT("Could not pack robot header\n");
								return -1;
							}
							
							l_position += l_sensedObject.size;
						}
						
					}
					
					if (l_curConnection->send(msgBuffer, l_responseMsgSize) == -1)
					{
						DEBUGPRINT("failed to send");
						return -1;
					}
					
					DEBUGPRINT("Sent Response\n");
						
					return 0;
					
				} // end of message sensor date
				
				case (MSG_PROCESSACTION):
				{
					/*int l_numRobots = -1;
					
					NetworkCommon::requestMessageSize(l_numRobots, l_curConnection);
					
					if (l_numRobots <= 0)
					{
						DEBUGPRINT("Requested sensor data for <=0 Robots. Game error.\n");
						return -1;
					}
					
					DEBUGPRINT("Client has requested actions for %i robots\n", l_numRobots);
					
					Msg_header l_header = {SENDER_GRIDSERVER, MSG_RESPONDSENSORDATA}; // header for response
					Msg_MsgSize l_msgSize;
					
					int l_position = l_header.size + l_msgSize.size;
					
					Msg_RobotId l_robotId;
					Msg_Action l_action;
					
					int l_msgSize = (l_robotId.size * l_numRobots) + (l_numRobots * l_action.size);
					
					unsigned char l_robotIdBuff[l_msgSize];
					
					for (int i = 0; i < l_numRobots; i++)
					{
						l_position = i*(l_robotId.size + l_action.size);
						
						if (l_curConnection->recv(l_robotIdBuff+l_position, l_robotId.size) == -1)
						{
							DEBUGPRINT("Could not get robot id for returnSensorData\n");
							return 0;
						}
						unsigned int id;
						unpack(l_robotIdBuff+l_position,Msg_RobotId_format, &id);

						if (l_curConnection->recv(l_robotIdBuff+l_position+l_robotId.size, l_action.size) == -1)
						{
							DEBUGPRINT("Could not get robot id for returnSensorData\n");
							return 0;
						}
						memset(&l_action, 0, l_action.size);
						unpack(l_robotIdBuff+l_position+l_robotId.size, Msg_Action_format, &l_action.action, &l_action.speed, &l_action.angle);
						
						
					}*/
					
					return 0;
					
				}// end of message process action
				
				case (MSG_PROCESSINITTEAM):
				{					
					Msg_header l_header;
					Msg_MsgSize l_msgSize;
					
					NetworkCommon::requestMessageSize(l_msgSize, l_curConnection);
					
					int l_numTeams = l_msgSize.msgSize;
					
					if (l_numTeams <= 0)
					{
						DEBUGPRINT("Requested sensor data for <=0 Robots. Game error.\n");
						return -1;
					}
					
					DEBUGPRINT("Client has requested actions for %i robots\n", l_numTeams);
					
					Msg_TeamId l_teamId;
					unsigned char l_teamIdBuff[l_teamId.size];
					
					memset(&l_teamId, 0 , l_teamId.size);
					memset(l_teamIdBuff, 0 , l_teamId.size);
					
					std::vector<int> l_teamIdsVec;
					std::map<int, std::vector<robot_info> > l_teamRobotsMap;
					
					for (int i = 0; i < l_numTeams; i++)
					{
						memset(l_teamIdBuff, 0 , l_teamId.size);
						if (l_curConnection->recv(l_teamIdBuff, l_teamId.size) == -1)
						{
							DEBUGPRINT("Could not receive a team id.");
							return -1;
						}
						unpack(l_teamIdBuff, Msg_TeamId_format, &l_teamId.teamId);
						
						l_teamIdsVec.push_back(l_teamId.teamId);
						
						DEBUGPRINT("Team id:%d unpacked and stored\n", l_teamId.teamId);
					}
					
					/*if (GridGame::initializeTeam(vector<int> teamids, map<int, vector<robot_info> >* robots) < 0)
					{
						DEBUGPRINT("Initialize Team\n");
						return -1;
					}*/
					
					return 0;
				}
				
				default:
				{
					DEBUGPRINT("No matching message handle for client case.\n");
					return -1;
				}
			} // end of sender
		default:
		{
			printf("no matching msg handler for UNKNOWN\n");
			return -1;
		} // end of default
		
	} // end of switch

    return 0;
}


