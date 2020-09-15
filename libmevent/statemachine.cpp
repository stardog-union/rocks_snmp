/**
 * @file statemachine.cpp
 * @author matthewv
 * @date May 9, 2011
 * @date Copyright 2011
 *
 * @brief Implementation of event/state machine for file descriptor operations
 */

#include "statemachine.h"
#include "util/logging.h"

/**
 * Initialize the data members.
 * @date 05/09/11  matthewv  Created
 */
StateMachine::StateMachine()
    : m_PrevNode(0), m_CurNode(0)
{
    return;
}   // StateMachine::StateMachine

/**
 * Release resources
 * @date 05/06/11  matthewv  Created
 */
StateMachine::~StateMachine()
{
    m_PrevNode=m_CurNode;
    m_CurNode=0;

    return;

}   // StateMachine::~StateMachine


/**
 * Share an edge notify with all watchers
 * @date 05/09/11  matthewv  Created
 */
bool
StateMachine::SendEdge(
    unsigned int EdgeId,   //!< edge for notification
    bool IsFinalState)     //!< send to both Change and Completion watchers
{
    bool used;
    StateMachinePtr self = GetStateMachinePtr();

    // send all change watchers pre
    // send all completion watchers pre, if IsFinalState

    // send to self
    used=EdgeNotification(EdgeId, self, true);

    // send all change watchers post
    // send all completion watchers post, if IsFinalState

    return(used);

}   // StateMachine::SendEdge


/**
 * Send edge notify to one specific object
 * @date 06/15/11  matthewv  Created
 */
bool
StateMachine::SendEdgeTo(
    StateMachine & Object, //!< object to receive the notification
    unsigned int EdgeId,   //!< edge for notification
    bool IsFinalState)     //!< send to both Change and Completion watchers
{
    bool used;
    StateMachinePtr self = GetStateMachinePtr();

    // send to self
    used=Object.EdgeNotification(EdgeId, self, true);

    return(used);

}   // StateMachine::SendEdgeTo


/**
 * Add a watcher to this object's closure list
 * @date Created 05/24/11
 * @author matthewv
 */
void
StateMachine::AddCompletion(
    StateMachinePtr & Add)           //!< object that desires closure messages
{
    std::pair<StateMachineSet_t::iterator, bool> ret_pair;
    StateMachineSet_t::iterator it;

    //m_CompletionList.push_back(Add);

#if 1
    it=m_CompletionList.insert(Add);
    // debug for now ...
    if (m_CompletionList.end()==it)
        Logging(LOG_ERR, "%s: AddCompletion failed", __func__);
#endif

    return;

}   // StateMachine::AddCompletion


/**
 * Remove a watcher to this object's closure list
 * @date Created 05/24/11
 * @author matthewv
 */
void
StateMachine::RemoveCompletion(
    StateMachinePtr & Remove)           //!< object that desires closure messages
{
    StateMachineSet_t::iterator it;

    // find the first (could be more than one, only delete one)
    it=m_CompletionList.find(Remove);

    if (m_CompletionList.end()!=it)
    {
        m_CompletionList.erase(it);

    }   // if
    else
    {
            Logging(LOG_ERR, "%s: RemoveCompletion failed 2", __func__);
    }   // else


    return;

}   // StateMachine::AddCompletion


/**
 * Notify all watcher of a completion event
 * @date created 05/30/11
 * @author matthewv
 */
void
StateMachine::SendCompletion(
    unsigned Event)                //!< completion event to send to all watchers
{
    StateMachineSet_t::iterator it;
    StateMachine * ptr;

    for (it=m_CompletionList.begin(); m_CompletionList.end()!=it; /* ++it below */)
    {

        ptr=(StateMachine *)(*it).get();

        // increment position now in case current it object deletes self
        ++it;

        StateMachinePtr this_obj = GetStateMachinePtr();

        ptr->EdgeNotification(Event, this_obj, false);
    }   // for

    return;

}   // StateMachine::SendCompletion
