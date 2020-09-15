/**
 * @file statemachine.h
 * @author matthewv
 * @date March 7, 2011
 * @date Copyright 2011-2012
 *
 * @brief State machine declarations
 */

#ifndef STATEMACHINE_H
#define STATEMACHINE_H

#include <memory>
#include <set>


/**
 * State machine is defined by nodes and edges.  Consider it
 *  a directed graph.
 *
 * Nodes and Edges have independent ID values.  But, the design
 * allows for any node id to be added/OR'd with an edge id to create
 * a single unsigned, 32 bit integer.  This is purely for convenience
 * of switch blocks.  But requires Node to by multiplied by 0x00010000
 * to use this convention (or shifted 16).
 *
 * Node and Edge values are assumed to be non-zero.
 */

#define STATEMACHINE_NODE_MASK 0xffff0000
#define STATEMACHINE_EDGE_MASK 0x0000ffff

/**
 * Source mask is an optional convention for filtering edge
 * messages from package specific sources.  Edge ids are NOT
 * required to follow this convention.  But must ensure they do
 * not overlap products using the convention.
 */

#define STATEMACHINE_EDGESRC_FLAG 0x00008000
#define STATEMACHINE_EDGESRC_MASK 0x0000ff00


/**
 * Object that characterizes state machine operation
 */

typedef std::shared_ptr<class StateMachine> StateMachinePtr;

class StateMachinePtrCompare
{
public:
    bool operator () (const StateMachinePtr & P1, const StateMachinePtr & P2) const
    {
        return(P1.get() < P2.get());
    }   // operator
};  // class StateMachineCompare

typedef std::multiset<StateMachinePtr, StateMachinePtrCompare> StateMachineSet_t;

class StateMachine : public std::enable_shared_from_this<StateMachine>
{
    /****************************************************************
    *  Member objects
    ****************************************************************/
public:

protected:
    unsigned int m_PrevNode;    //!< node before current node, or zero
    unsigned int m_CurNode;     //!< node most recently traversed by edge

    // ? std::vector<StateMachinePtr> for watchers on any state change ... ref ptr?
    StateMachineSet_t m_CompletionList;  //!< for completion watchers
                                              // "completion" is defined object by object
private:

    /****************************************************************
    *  Member functions
    ****************************************************************/
public:

    /// Initialize object to default state
    StateMachine();

    /// Release resources, put values in a "debug" state
    virtual ~StateMachine();

    // create a way to self reference a common shared_ptr to this object
    StateMachinePtr GetStateMachinePtr() {return shared_from_this();}

    /// Public routine to receive Edge notification
    virtual bool EdgeNotification(unsigned int EdgeId, StateMachinePtr & Caller, bool PreNotify)
    {return(false);};

    /// debug
    void Dump()
    {printf("Current node: %u, previous: %u\n", m_CurNode, m_PrevNode);};

    /// comparison to allow std::set construction
    bool operator<(const StateMachine & rhs) const
        {return(this<&rhs);};

    /// add watcher to this object's closure list
    void AddCompletion(StateMachinePtr & Add);
    //    void AddCompletion(StateMachine & Add)
    //{StateMachinePtr state; state=Add; AddCompletion(state);};

    /// remove watcher from this object's closure list
    void RemoveCompletion(StateMachinePtr & Remove);

    /// send a completion event to all registered
    void SendCompletion(unsigned Event);

    /// put state machine into a new state
    virtual void SetState(unsigned int StateId)
        {m_PrevNode=m_CurNode; m_CurNode=StateId;};

    /// simple accessor to get current state value (m_CurNode)
    unsigned int GetState() const {return(m_CurNode);};


protected:
    /// initiate the sending of an edge change to all interested parties
    virtual bool SendEdge(unsigned int EdgeId, bool IsFinalState=false);

    /// initiate the sending of an edge change to specific object
    virtual bool SendEdgeTo(StateMachine & Object, unsigned int EdgeId,
                            bool IsFinalState=false);

private:
    StateMachine(const StateMachine & );            //!< disabled:  copy operator
    StateMachine & operator=(const StateMachine &); //!< disabled:  assignment operator

};   // class StateMachine

#endif // ifndef STATEMACHINE_H
