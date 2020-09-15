/**
 * @file snmp_value.cpp
 * @author matthewv
 * @date August 31, 2011
 * @date Copyright 2011-2012
 *
 * @brief Implementation of snmp agentX value objects (rfc 2741, January 2000)
 */

#ifndef SNMP_VALUE_H
    #include "snmp_value.h"
#endif

#ifndef VAL_INTEGER_H
    #include "val_integer.h"
#endif

#ifndef VAL_INTEGER64_H
    #include "val_integer64.h"
#endif

#ifndef VAL_STRING_H
    #include "val_string.h"
#endif


struct VarBindHeader gVarNoSuchObject={eNoSuchObject,0};
struct VarBindHeader gVarEndOfMibView={eEndOfMibView,0};


/**
 * Initialize without suffix
 * @date Created 11/20/12
 * @author matthewv
 */
SnmpValInf::SnmpValInf()
    : m_PrefixSet(false)
{

    Init();

    return;

}   // SnmpValInf::SnmpValInf


/**
 * Initialize the data members.
 * @date Created 08/31/11
 * @author matthewv
 */
SnmpValInf::SnmpValInf(
    const SnmpOid & Suffix)
    : m_PrefixSet(false)
{
    if (NULL!=Suffix.m_Oid)
    {
        size_t loop;

        m_Oid.reserve(Suffix.m_OidLen);
        for (loop=0; loop<Suffix.m_OidLen; ++loop)
            m_Oid.push_back(Suffix.m_Oid[loop]);
    }   // if
    else
    {
        // Error Oids have no suffix
        // Logging(LOG_ERR, "%s: bad parameter", __func__);
    }   // else

    Init();

    return;

}   // SnmpValInf::SnmpValInf


/**
 * Initialize the data members.
 * @date Created 12/10/11
 * @author matthewv
 */
SnmpValInf::SnmpValInf(
    const OidVector_t & Suffix)
    : m_Oid(Suffix), m_PrefixSet(false)
{
    Init();

    return;

}   // SnmpValInf::SnmpValInf


/**
 * Initialize the data members.
 * @date Created 12/10/11
 * @author matthewv
 */
SnmpValInf::SnmpValInf(
    unsigned Suffix)
    : m_PrefixSet(false)
{
    m_Oid.push_back(Suffix);
    Init();

    return;

}   // SnmpValInf::SnmpValInf


/**
 * Common routine for all constructors
 * @date Created 02/12/12
 * @author matthewv
 */
void
SnmpValInf::Init()
{
    // default usage is a variable is within another object,
    //   RefPtr() should not auto delete
  //    MarkNotHeap();

    // default is static / direct access variable, not lookups
  //SetState(SNMP_NODE_DATAGOOD);

    return;
}   // SnmpValInf::Init


/**
 * Assign suffix late, after default constructor
 * @date Created 11/20/12
 * @author matthewv
 */
void
SnmpValInf::SetSuffix(
    const SnmpOid & Suffix)
{
    if (NULL!=Suffix.m_Oid)
    {
        size_t loop;

        m_Oid.reserve(Suffix.m_OidLen);
        for (loop=0; loop<Suffix.m_OidLen; ++loop)
            m_Oid.push_back(Suffix.m_Oid[loop]);
    }   // if
    else
    {
        // Error Oids have no suffix
        // Logging(LOG_ERR, "%s: bad parameter", __func__);
    }   // else
}   // SnmpValInf::SetSuffix

/**
 * Glue front agent oid to this objects
 * @date Created 12/09/11
 * @author matthewv
 */
void
SnmpValInf::InsertPrefix(
    const OidVector_t & OidPrefix)
{
    if (!m_PrefixSet)
    {
        m_Oid.insert(m_Oid.begin(), OidPrefix.begin(), OidPrefix.end());
        m_PrefixSet=true;

        m_SubId.m_SubIdLen=m_Oid.size();
        m_SubId.m_Prefix=4;
        m_SubId.m_Index=0;
        m_SubId.m_Reserved2=0;
    }   // if

    return;

}   // SnmpValInf::InsertPrefix


/**
 * Table Oid needs several items for construction
 * @date Created 12/10/11
 * @author matthewv
 */
void
SnmpValInf::InsertTablePrefix(
    const OidVector_t & OidAgentPrefix,      //!< true OID prefix
    const OidVector_t & OidTablePrefix,      //!< table OID that follows agent
    const OidVector_t & OidAreaPrefix,       //!< sub-area of the table
    const OidVector_t & OidRowIdSuffix)      //!< row identifier (could be zero length)
{
    if (!m_PrefixSet)
    {
        size_t temp;

        // resize once
        temp=m_Oid.size()+OidAgentPrefix.size()+OidTablePrefix.size()+OidAreaPrefix.size()
            +OidRowIdSuffix.size();
        m_Oid.reserve(temp);

        // push values like a stack, 3rd position, 2nd, then first
        m_Oid.insert(m_Oid.begin(), OidAreaPrefix.begin(), OidAreaPrefix.end());
        m_Oid.insert(m_Oid.begin(), OidTablePrefix.begin(), OidTablePrefix.end());
        m_Oid.insert(m_Oid.begin(), OidAgentPrefix.begin(), OidAgentPrefix.end());

        // now suffix
        m_Oid.insert(m_Oid.end(), OidRowIdSuffix.begin(), OidRowIdSuffix.end());
        m_PrefixSet=true;

        m_SubId.m_SubIdLen=m_Oid.size();
        m_SubId.m_Prefix=4;
        m_SubId.m_Index=0;
        m_SubId.m_Reserved2=0;

    }   // if

    return;

}   // SnmpValInf::InsertTablePrefix


/**
 * Is data currently good?  If not ask for update.
 * @date 01/19/12 Created
 * @author matthewv
 */
bool
SnmpValInf::IsDataReady(
    class StateMachine * Notify)    //!< a potentially transient object to notify later
{
    bool ret_flag;

    ret_flag=(true /*SNMP_NODE_DATAGOOD==GetState()*/);

    // someone wants data, ask for update
    //    if (!ret_flag)
    //  SendCompletion(SNMP_EDGE_REQUEST_DATA);

    // yes test again in case update happened synchronously
    ret_flag=(true /*SNMP_NODE_DATAGOOD==GetState()*/);

    // save Notify only if we owe them fresh data
    //    if (!ret_flag && NULL!=Notify)
    //  AddCompletion(*Notify);

    return(ret_flag);

}   // SnmpValInf::IsDataReady


/**
 * Turn edge notifications into State changes
 *
 * @date Created 01/19/12
 * @author matthewv
 * @returns  true if edge handled to state transition
 */
#if 0
bool
SnmpValInf::EdgeNotification(
    unsigned int EdgeId,               //!< what just happened, what graph edge are we walking
    StateMachinePtr & Caller,          //!< what state machine object initiated the edge
    bool PreNotify)                    //!< for watchers, is the before or after owner processes
{
    bool used;

    used=false;

    // only care about EXTERNAL events
    if (this!=Caller.get())
    {
        switch(EdgeId)
        {
            // external source says our data good
            case SNMP_EDGE_DATA_GOOD:
                // tell watchers of this object that data is now good
                SetState(SNMP_NODE_DATAGOOD);
                SendCompletion(SNMP_EDGE_DATA_GOOD);
                SendCompletion(ReaderWriter::RW_EDGE_DATAREADY);
                used=true;
                break;

                // external says data old
            case SNMP_EDGE_DATA_STALE:
                SetState(SNMP_NODE_DATASTALE);
                used=true;
                break;

            default:
                // ignore any other notifications
                break;
        }   // switch
    }   // if

    return(used);

}   // SnmpValInf::EdgeNotifications
#endif


/**
 * Output contents of object to console (for debugging)
 * @date Created 12/17/11
 * @author matthewv
 */
void
SnmpValInf::SnmpDump() const
{
    OidVector_t::const_iterator v_it;

    printf("  Oid: ");
    for (v_it=m_Oid.begin(); m_Oid.end()!=v_it; ++v_it)
    {
        if (m_Oid.begin()!=v_it)
            printf(", ");

        printf("%u", *v_it);
    }   // for
    printf("\n");

    return;

}   // SnmpValInf::SnmpDump


/**
 * Static function to create various SNMP value types
 * @date Created 02/21/12
 * @author matthewv
 */
SnmpValInfPtr
SnmpValInf::ValueFactory(ValueTypeCodes_e Type)
{
    SnmpValInfPtr ret_ptr;
    OidVector_t null_oid;

    switch(Type)
    {
        case eInteger:
          ret_ptr=std::make_shared<SnmpValInteger>(null_oid);
            break;

        case eGauge32:
          ret_ptr=std::make_shared<SnmpValGauge32>(null_oid);
            break;

        case eCounter64:
          ret_ptr=std::make_shared<SnmpValCounter64>(null_oid);
            break;

        case eOctetString:
          ret_ptr=std::make_shared<SnmpValString>(null_oid);
            break;

        default:
            Logging(LOG_ERR, "%s:  unsupported type code passed [Type %d]",
                    __func__, Type);
            break;
    }   // switch

    // snmp values default to stack/member based
    if (NULL!=ret_ptr.get())
    {
      //        ret_ptr.MarkHeap();
    }   // if

    return(ret_ptr);

}   // SnmpValInf::ValueFactory
