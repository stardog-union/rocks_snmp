/**
 * @file val_table.cpp
 * @author matthewv
 * @date November 26, 2011
 * @date Copyright 2011-2012
 *
 * @brief Integer based snmp value
 */

#ifndef VAL_TABLE_H
    #include "val_table.h"
#endif


static const unsigned sRowCount[]={1,1};
static SnmpOid sRowCountId={sRowCount, sizeof(sRowCount)/sizeof(sRowCount[0])};
/** maybe table description has 1.2 some day **/



/**
 * Initialize the data members.
 * @date Created 11/26/11
 * @author matthewv
 */
SnmpValTable::SnmpValTable(
    SnmpAgent & Agent,       //!< agent to manage this data
    SnmpOid & TablePrefix)   //!< oid fragment between prefix and data
    : m_Agent(Agent)
{
    unsigned loop;
    OidVector_t oid_area, oid_null;
    m_RowCount = std::make_shared<SnmpValInteger>(sRowCountId);

    oid_area.push_back(1);
    m_TablePrefix.reserve(TablePrefix.m_OidLen);
    for (loop=0; loop<TablePrefix.m_OidLen; ++loop)
        m_TablePrefix.push_back(TablePrefix.m_Oid[loop]);

    m_RowCount->InsertTablePrefix(Agent.GetOidPrefix(), m_TablePrefix, oid_area, oid_null);
    SnmpValInfPtr temp = m_RowCount;
    m_Agent.AddVariable(temp);

    return;

}   // SnmpValTable::SnmpValTable


/**
 * Put integer row id into row index
 * @date Create 12/10/11
 * @author matthewv
 */
bool
SnmpValTable::AddRow(
    int RowId)
{
    SnmpValUnsigned32Ptr new_row;
    SnmpValInfPtr inf_ptr;
    OidVector_t row_index, row_area, row_oid;
    bool flag;

    flag=false;

    // (table).(area 1).2
    row_area.push_back(1);
    row_oid.push_back(2);

    new_row=std::make_shared<SnmpValInteger>(row_oid);

    if (NULL!=new_row.get())
    {
        // snmp's default is non-heap
      //        new_row.MarkHeap();

      ++(*m_RowCount);
        row_index.push_back(m_RowCount->unsigned32());

        new_row->assign(RowId);

        m_RowXlate.push_back(new_row);
        new_row->InsertTablePrefix(m_Agent.GetOidPrefix(), m_TablePrefix,
                                   row_area, row_index);

        inf_ptr=new_row;
        m_Agent.AddVariable(inf_ptr);

        flag=true;
    }   // if

    return(flag);

}   // SnmpValTable::AddRow
