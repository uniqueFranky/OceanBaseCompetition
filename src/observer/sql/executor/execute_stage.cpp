/* Copyright (c) 2021 Xie Meiyi(xiemeiyi@hust.edu.cn) and OceanBase and/or its affiliates. All rights reserved.
miniob is licensed under Mulan PSL v2.
You can use this software according to the terms and conditions of the Mulan PSL v2.
You may obtain a copy of Mulan PSL v2 at:
         http://license.coscl.org.cn/MulanPSL2
THIS SOFTWARE IS PROVIDED ON AN "AS IS" BASIS, WITHOUT WARRANTIES OF ANY KIND,
EITHER EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO NON-INFRINGEMENT,
MERCHANTABILITY OR FIT FOR A PARTICULAR PURPOSE.
See the Mulan PSL v2 for more details. */

//
// Created by Longda on 2021/4/13.
//

#include <cstring>
#include <map>
#include <algorithm>
#include "execute_stage.h"

#include "common/io/io.h"
#include "common/log/log.h"
#include "common/seda/timer_stage.h"
#include "common/lang/string.h"
#include "session/session.h"
#include "event/storage_event.h"
#include "event/sql_event.h"
#include "event/session_event.h"
#include "event/execution_plan_event.h"
#include "sql/executor/execution_node.h"
#include "sql/executor/tuple.h"
#include "storage/common/table.h"
#include "storage/default/default_handler.h"
#include "storage/common/condition_filter.h"
#include "storage/trx/trx.h"

using namespace common;
void record_reader(const char *data, void *context);
RC create_selection_executor(Trx *trx, const Selects &selects, const char *db, const char *table_name, SelectExeNode &select_node);

//! Constructor
ExecuteStage::ExecuteStage(const char *tag) : Stage(tag) {}

//! Destructor
ExecuteStage::~ExecuteStage() {}

//! Parse properties, instantiate a stage object
Stage *ExecuteStage::make_stage(const std::string &tag) {
  ExecuteStage *stage = new (std::nothrow) ExecuteStage(tag.c_str());
  if (stage == nullptr) {
    LOG_ERROR("new ExecuteStage failed");
    return nullptr;
  }
  stage->set_properties();
  return stage;
}

//! Set properties for this object set in stage specific properties
bool ExecuteStage::set_properties() {
  //  std::string stageNameStr(stageName);
  //  std::map<std::string, std::string> section = theGlobalProperties()->get(
  //    stageNameStr);
  //
  //  std::map<std::string, std::string>::iterator it;
  //
  //  std::string key;

  return true;
}

//! Initialize stage params and validate outputs
bool ExecuteStage::initialize() {
  LOG_TRACE("Enter");

  std::list<Stage *>::iterator stgp = next_stage_list_.begin();
  default_storage_stage_ = *(stgp++);
  mem_storage_stage_ = *(stgp++);

  LOG_TRACE("Exit");
  return true;
}

//! Cleanup after disconnection
void ExecuteStage::cleanup() {
  LOG_TRACE("Enter");

  LOG_TRACE("Exit");
}

void ExecuteStage::handle_event(StageEvent *event) {
  LOG_TRACE("Enter\n");

  handle_request(event);

  LOG_TRACE("Exit\n");
  return;
}

void ExecuteStage::callback_event(StageEvent *event, CallbackContext *context) {
  LOG_TRACE("Enter\n");

  // here finish read all data from disk or network, but do nothing here.
  ExecutionPlanEvent *exe_event = static_cast<ExecutionPlanEvent *>(event);
  SQLStageEvent *sql_event = exe_event->sql_event();
  sql_event->done_immediate();

  LOG_TRACE("Exit\n");
  return;
}

void ExecuteStage::handle_request(common::StageEvent *event) {
  ExecutionPlanEvent *exe_event = static_cast<ExecutionPlanEvent *>(event);
  SessionEvent *session_event = exe_event->sql_event()->session_event();
  Query *sql = exe_event->sqls();
  const char *current_db = session_event->get_client()->session->get_current_db().c_str();

  CompletionCallback *cb = new (std::nothrow) CompletionCallback(this, nullptr);
  if (cb == nullptr) {
    LOG_ERROR("Failed to new callback for ExecutionPlanEvent");
    exe_event->done_immediate();
    return;
  }
  exe_event->push_callback(cb);

  switch (sql->flag) {
    case SCF_SELECT: { // select
      do_select(current_db, sql, exe_event->sql_event()->session_event());
      exe_event->done_immediate();
    }
    break;

    case SCF_INSERT:
    case SCF_UPDATE:
    case SCF_DELETE:
    case SCF_CREATE_TABLE:
    case SCF_SHOW_TABLES:
    case SCF_DESC_TABLE:
    case SCF_DROP_TABLE:
    case SCF_CREATE_INDEX:
    case SCF_DROP_INDEX: 
    case SCF_LOAD_DATA: {
      StorageEvent *storage_event = new (std::nothrow) StorageEvent(exe_event);
      if (storage_event == nullptr) {
        LOG_ERROR("Failed to new StorageEvent");
        event->done_immediate();
        return;
      }

      default_storage_stage_->handle_event(storage_event);
    }
    break;
    case SCF_SYNC: {
      RC rc = DefaultHandler::get_default().sync();
      session_event->set_response(strrc(rc));
      exe_event->done_immediate();
    }
    break;
    case SCF_BEGIN: {
      session_event->get_client()->session->set_trx_multi_operation_mode(true);
      session_event->set_response(strrc(RC::SUCCESS));
      exe_event->done_immediate();
    }
    break;
    case SCF_COMMIT: {
      Trx *trx = session_event->get_client()->session->current_trx();
      RC rc = trx->commit();
      session_event->get_client()->session->set_trx_multi_operation_mode(false);
      session_event->set_response(strrc(rc));
      exe_event->done_immediate();
    }
    break;
    case SCF_ROLLBACK: {
      Trx *trx = session_event->get_client()->session->current_trx();
      RC rc = trx->rollback();
      session_event->get_client()->session->set_trx_multi_operation_mode(false);
      session_event->set_response(strrc(rc));
      exe_event->done_immediate();
    }
    break;
    case SCF_HELP: {
      const char *response = "show tables;\n"
          "desc `table name`;\n"
          "create table `table name` (`column name` `column type`, ...);\n"
          "create index `index name` on `table` (`column`);\n"
          "insert into `table` values(`value1`,`value2`);\n"
          "update `table` set column=value [where `column`=`value`];\n"
          "delete from `table` [where `column`=`value`];\n"
          "select [ * | `columns` ] from `table`;\n";
      session_event->set_response(response);
      exe_event->done_immediate();
    }
    break;
    case SCF_EXIT: {
      // do nothing
      const char *response = "Unsupported\n";
      session_event->set_response(response);
      exe_event->done_immediate();
    }
    break;
    default: {
      exe_event->done_immediate();
      LOG_ERROR("Unsupported command=%d\n", sql->flag);
    }
  }
}

void end_trx_if_need(Session *session, Trx *trx, bool all_right) {
  if (!session->is_trx_multi_operation_mode()) {
    if (all_right) {
      trx->commit();
    } else {
      trx->rollback();
    }
  }
}
std::vector<TupleSet> tuple_sets;
std::vector<TupleSet> out;
int last_num;
/*
 EQUAL_TO,     //"="     0
 LESS_EQUAL,   //"<="    1
 NOT_EQUAL,    //"<>"    2
 LESS_THAN,    //"<"     3
 GREAT_EQUAL,  //">="    4
 GREAT_THAN,   //">"     5
 */


bool check(void *lval, void *rval, AttrType type, CompOp op)
{
    if(type == INTS || type == DATES)
    {
            int l = *(int *)lval;
            int r = *(int *)rval;
            switch (op)
            {
                case EQUAL_TO:
                    return l == r;
                    break;
                case LESS_EQUAL:
                    return l <= r;
                    break;
                case NOT_EQUAL:
                    return l != r;
                    break;
                case LESS_THAN:
                    return l < r;
                    break;
                case GREAT_EQUAL:
                    return l >= r;
                    break;
                case GREAT_THAN:
                    return l > r;
                    break;
                default:
                    break;
            }
    }
    if(type == FLOATS)
    {
            float l = *(float *)lval;
            float r = *(float *)rval;
            switch (op)
            {
                case EQUAL_TO:
                    return l == r;
                    break;
                case LESS_EQUAL:
                    return l <= r;
                    break;
                case NOT_EQUAL:
                    return l != r;
                    break;
                case LESS_THAN:
                    return l < r;
                    break;
                case GREAT_EQUAL:
                    return l >= r;
                    break;
                case GREAT_THAN:
                    return l > r;
                    break;
                default:
                    break;
            }
    }
    if(type == CHARS)
    {
            std::string l = *(std::string *)lval;
            std::string r = *(std::string *)rval;
            switch (op)
            {
                case EQUAL_TO:
                    return l == r;
                    break;
                case LESS_EQUAL:
                    return l <= r;
                    break;
                case NOT_EQUAL:
                    return l != r;
                    break;
                case LESS_THAN:
                    return l < r;
                    break;
                case GREAT_EQUAL:
                    return l >= r;
                    break;
                case GREAT_THAN:
                    return l > r;
                    break;
                default:
                    break;
            }
    }
    return false;
}

std::map<std::string, bool> printed;
RC dfs(int now, Tuple *cur_tuple, Selects selects, std::map<std::string, int> mp, std::map<int, bool> need, TupleSchema tot_schema, TupleSchema out_schema, std::ostream &ss)
{
    if(now == tuple_sets.size())
    {
        
        for(int i = 0; i < selects.condition_num; i++)
        {
            Condition &cond = selects.conditions[i];
            Value lval, rval;
            if(cond.left_is_attr)
            {
                char *tname = cond.left_attr.relation_name;
                char *fname = cond.left_attr.attribute_name;
                char tot_name[10000] = "";
                strcat(tot_name, tname);
                strcat(tot_name, fname);
                std::string ttname (tot_name);
                int id = mp[ttname];
                AttrType type = tot_schema.field(id).type();
                lval.type = type;
                lval.data =(void *)((*(cur_tuple->values()[id])).get_value());
            }
            else
            {
                if(CHARS != cond.left_value.type)
                    lval = cond.left_value;
                else
                {
                    std::string t ((char*)cond.left_value.data);
                    lval.type = CHARS;
                    lval.data = (void *)&t;
                }
            }
            
            if(cond.right_is_attr)
            {
                char *tname = cond.right_attr.relation_name;
                char *fname = cond.right_attr.attribute_name;
                char tot_name[10000] = "";
                strcat(tot_name, tname);
                strcat(tot_name, fname);
                std::string ttname (tot_name);
                int id = mp[ttname];
                AttrType type = tot_schema.field(id).type();
                rval.type = type;
                rval.data =(void *)((*(cur_tuple->values()[id])).get_value());
            }
            else
            {
                if(CHARS != cond.right_value.type)
                    rval = cond.right_value;
                else
                {
                    std::string t ((char*)cond.right_value.data);
                    rval.type = CHARS;
                    rval.data = (void *)&t;
                }
            }
            if(lval.type != rval.type)
                return RC::SCHEMA_FIELD_TYPE_MISMATCH;
            if(false == check(lval.data, rval.data, lval.type, cond.comp))
                return RC::SUCCESS;
        }
        std::string t = "";
        std::stringstream ts;
        //judge
//        std::cout<<cur_tuple->size()<<std::endl;
        for(int i = 0; i < cur_tuple->size(); i++)
        {
            const TupleField field = tot_schema.field(i);
            char tot_name[10000] = "";
            strcat(tot_name, field.table_name());
            strcat(tot_name, field.field_name());
//            std::cout<<tot_name<<std::endl;
            std::string ttname (tot_name);
            if(false == need[mp[tot_name]])
                continue;
            AttrType type = tot_schema.field(i).type();
            switch (type)
            {
                case INTS:
                    ts<<*(int *)((*(cur_tuple->values()[i])).get_value());
                    break;
                case FLOATS:
                    ts<<*(float *)((*(cur_tuple->values()[i])).get_value());
                    break;
                case CHARS:
                    ts<<*(std::string *)((*(cur_tuple->values()[i])).get_value());
                    break;
                case DATES:
                {
                    int x = *(int *)((*(cur_tuple->values()[i])).get_value());;
                    int y = x / 10000;
                    int m = x / 100 % 100;
                    int d = x % 100;
                    ts<<y<<"-";
                    if(m < 10)
                        ts<<"0";
                    ts<<m<<"-";
                    if(d < 10)
                        ts<<"0";
                    ts<<d;
                    break;
                }
                default:
                    break;
            }
            if(i == last_num)
                ts<<std::endl;
            else
                ts<<" | ";
        }
        
        t = ts.str();
        if(!printed[t])
        {
            printed[t] = true;
            ss<<ts.str();
        }
        return RC::SUCCESS;
    }
    for(int i = 0; i < tuple_sets[now].size(); i++)
    {
        Tuple tmp;
        for(int j = 0; j < cur_tuple->size(); j++)
            tmp.add(cur_tuple->values()[j]);
        
        const Tuple *tp = &(tuple_sets[now].tuples()[i]);
        for(int j = 0; j < tp->size(); j++)
            tmp.add(tp->values()[j]);
        if(RC::SUCCESS != dfs(now + 1, &tmp, selects, mp, need, tot_schema, out_schema, ss))
            return RC::SCHEMA_FIELD_TYPE_MISMATCH;
    }
    return RC::SUCCESS;
}



// 这里没有对输入的某些信息做合法性校验，比如查询的列名、where条件中的列名等，没有做必要的合法性校验
// 需要补充上这一部分. 校验部分也可以放在resolve，不过跟execution放一起也没有关系
RC ExecuteStage::do_select(const char *db, Query *sql, SessionEvent *session_event) {

    std::stringstream ss;
    tuple_sets.clear();
    out.clear();
  RC rc = RC::SUCCESS;
  Session *session = session_event->get_client()->session;
  Trx *trx = session->current_trx();
  const Selects &selects = sql->sstr.selection;
//    std::cout<<"@@"<<selects.aggrega_num<<std::endl;
  // 把所有的表和只跟这张表关联的condition都拿出来，生成最底层的select 执行节点
          std::vector<SelectExeNode *> select_nodes;
          for (size_t i = 0; i < selects.relation_num; i++) {
            const char *table_name = selects.relations[i];
            SelectExeNode *select_node = new SelectExeNode;
            rc = create_selection_executor(trx, selects, db, table_name, *select_node);
            if (rc != RC::SUCCESS) {
              delete select_node;
              for (SelectExeNode *& tmp_node: select_nodes) {
                delete tmp_node;
              }
        //      if(RC::SCHEMA_TABLE_NOT_EXIST == rc || RC::SCHEMA_FIELD_NOT_EXIST == rc || RC::SCHEMA_FIELD_MISSING == rc || RC::SCHEMA_FIELD_TYPE_MISMATCH == rc)
                  session_event->set_response("FAILURE\n");
              end_trx_if_need(session, trx, false);
              return rc;
            }
            select_nodes.push_back(select_node);
          }

          if (select_nodes.empty()) {
            LOG_ERROR("No table given");
            end_trx_if_need(session, trx, false);
            return RC::SQL_SYNTAX;
          }

          for (SelectExeNode *&node: select_nodes) {
            TupleSet tuple_set;
            rc = node->execute(tuple_set);
            if (rc != RC::SUCCESS) {
              for (SelectExeNode *& tmp_node: select_nodes) {
                delete tmp_node;
              }
              end_trx_if_need(session, trx, false);
              return rc;
            } else {
              tuple_sets.push_back(std::move(tuple_set));
            }
          }

          if (tuple_sets.size() > 1)
          {
              printed.clear();
              last_num = -1;
              tuple_sets.clear();
              std::map<std::string, int> mp;
              std::map<int, bool> need;
              int cnt = 0;
              TupleSchema schema, out_schema;
              
              for(int i = selects.relation_num - 1; i >= 0; i--)
              {
                  Table *table = DefaultHandler::get_default().find_table(db, selects.relations[i]);
                  if(nullptr == table)
                      return RC::SCHEMA_TABLE_NOT_EXIST;
                  for(size_t j = 1; j < table->table_meta().field_num(); j++)
                  {
                      const FieldMeta *field = table->table_meta().field(j);
                      schema.add(field->type(), table->name(), field->name());
                      const char *tname = table->name(), *fname = field->name();
                      char totname[10000] = "";
                      strcat(totname, tname);
                      strcat(totname, fname);
                      std::string ttname (totname);
                      mp[ttname] = cnt++;
                      for(int k = 0; k < selects.attr_num; k++)
                          if(nullptr == selects.attributes[k].relation_name||
                             0 == strcmp(selects.attributes[k].relation_name, tname))
                              if(0 == strcmp("*", selects.attributes[k].attribute_name) ||
                                 0 == strcmp(fname, selects.attributes[k].attribute_name))
                              {
                                  out_schema.add(field->type(), table->name(), field->name());
                                  need[mp[totname]] = true;
                                  last_num = std::max(mp[totname], last_num);
                              }
                  }
                  TupleSet tuple_set;
                  TupleSchema cur_schema;
                  TupleSchema::from_table(table, cur_schema);
                  tuple_set.set_schema(cur_schema);
                  TupleRecordConverter converter(table, tuple_set);
                  table->scan_record(trx, nullptr, -1, (void *)&converter, record_reader);
                  
                  tuple_sets.push_back(std::move(tuple_set));
              }
              out_schema.print(ss, true);
              Tuple cur;
              if(RC::SUCCESS != dfs(0, &cur, selects, mp, need, schema, out_schema, ss))
              {
                  session_event->set_response("FAILURE\n");
                  end_trx_if_need(session, trx, true);
                  return RC::SCHEMA_FIELD_TYPE_MISMATCH;
              }
              
          }
          else
          {
            // 当前只查询一张表，直接返回结果即可
            if(0 == selects.aggrega_num)
                tuple_sets.front().print(ss);
            else
            {
                TupleSchema aggr_schema;
                TupleSet tps = std::move(tuple_sets.front());
                TupleSet out_set;
                Tuple out_tuple;
                void *cur_values[1000];
                Table *table = DefaultHandler::get_default().find_table(db, selects.relations[0]);
                for(int i = 0; i < selects.aggrega_num; i++)
                {
                    const FieldMeta *field = table->table_meta().field(selects.attributes[i].attribute_name);
                    Aggregation aggr_type = selects.aggre_t[i];
                    for(int j = 0; j < tps.tuples().size(); j++)
                    {
                        const Tuple &item = tps.tuples()[j];
                        cur_values[j] = item.get(selects.aggrega_num - i - 1).get_value();
                    }
                    
                    switch (aggr_type)
                    {
                    /*************  case MAX_T  begin **************** */
                        case max_t:
                        {
                            char aggr_name[100] = "";
                            strcat(aggr_name, "max(");
                            strcat(aggr_name, field->name());
                            strcat(aggr_name, ")");
                            aggr_schema.add(field->type(), table->name(), aggr_name);
                            if(0 == tps.tuples().size())
                                break;
                            
                            switch(field->type())
                            {
                                case INTS:
                                {
                                    int ans = *(int *)cur_values[0];
                                    for(int j = 1; j < tps.tuples().size(); j++)
                                        ans = ans > (*(int *)cur_values[j]) ? ans : (*(int *)cur_values[j]);
                                    out_tuple.add(ans);
                                    break;
                                }
                                case FLOATS:
                                {
                                    float ans = *(float *)cur_values[0];
                                    for(int j = 1; j < tps.tuples().size(); j++)
                                        ans = ans > *(float *)cur_values[j] ? ans : *(float *)cur_values[j];
                                    out_tuple.add(ans);
                                    break;
                                }
                                case CHARS:
                                {
                                    char *ans = (char *)cur_values[0];
                                    for(int j = 1; j < tps.tuples().size(); j++)
                                        if(strcmp(ans, (char *)cur_values[j]) < 0)
                                            memcpy(ans, (char *)cur_values[j], strlen((char *)cur_values[j]));
                                    out_tuple.add(ans, strlen(ans));
                                    break;
                                }
                                case DATES:
                                {
                                    int ans = *(int *)cur_values[0];
                                    for(int j = 1; j < tps.tuples().size(); j++)
                                        ans = ans > *(int *)cur_values[j] ? ans : *(int *)cur_values[j];
                                    out_tuple.add(ans);
                                    break;
                                }
                                default:
                                    break;
                                    
                            }
                            
                            break;
                        }
                            /*************  case MAX_T  end **************** */

                            
                            /*************  case MIN_T  begin **************** */

                        case min_t:
                        {
                            char aggr_name[100] = "";
                            strcat(aggr_name, "min(");
                            strcat(aggr_name, field->name());
                            strcat(aggr_name, ")");
                            aggr_schema.add(field->type(), table->name(), aggr_name);
                            if(0 == tps.tuples().size())
                                break;
                            
                            switch(field->type())
                            {
                                case INTS:
                                {
                                    int ans = *(int *)cur_values[0];
                                    for(int j = 1; j < tps.tuples().size(); j++)
                                        ans = ans < *(int *)cur_values[j] ? ans : *(int *)cur_values[j];
                                    out_tuple.add(ans);
                                    break;
                                }
                                case FLOATS:
                                {
                                    float ans = *(float *)cur_values[0];
                                    for(int j = 1; j < tps.tuples().size(); j++)
                                        ans = ans < *(float *)cur_values[j] ? ans : *(float *)cur_values[j];
                                    out_tuple.add(ans);
                                    break;
                                }
                                case CHARS:
                                {
                                    char *ans = (char *)cur_values[0];
                                    for(int j = 1; j < tps.tuples().size(); j++)
                                        if(strcmp(ans, (char *)cur_values[j]) > 0)
                                            memcpy(ans, (char *)cur_values[j], 4);
                                    out_tuple.add(ans, 4);
                                    break;
                                }
                                case DATES:
                                {
                                    int ans = *(int *)cur_values[0];
                                    for(int j = 1; j < tps.tuples().size(); j++)
                                        ans = ans < *(int *)cur_values[j] ? ans : *(int *)cur_values[j];
                                    out_tuple.add(ans);
                                    break;
                                }
                                default:
                                    break;
                                    
                            }
                            
                            break;
                        }
                            /*************  case MIN_T  end **************** */

                            /*************  case COUNT_T  begin **************** */
                        case count_t:
                        {
                            char aggr_name[100] = "";
                            strcat(aggr_name, "count(");
                            strcat(aggr_name, selects.attributes[i].attribute_name);
                            strcat(aggr_name, ")");
                            aggr_schema.add(INTS, table->name(), aggr_name);
                            out_tuple.add((int)tps.tuples().size());
                            break;
                        }
                            /*************  case COUNT_T  end **************** */

                            /*************  case AVG_T  begin **************** */

                        case avg_t:
                        {
                            char aggr_name[100] = "";
                            strcat(aggr_name, "avg(");
                            strcat(aggr_name, field->name());
                            strcat(aggr_name, ")");
                            aggr_schema.add(FLOATS, table->name(), aggr_name);
                            if(0 == tps.tuples().size())
                                break;
                            
                            switch(field->type())
                            {
                                case INTS:
                                {
                                    float ans = *(int *)cur_values[0];
                                    for(int j = 1; j < tps.tuples().size(); j++)
                                        ans += *(int *)cur_values[j];
                                    ans /= tps.tuples().size();
                                    out_tuple.add(ans);
                                    
                                    break;
                                }
                                case FLOATS:
                                {
                                    float ans = *(float *)cur_values[0];
                                    for(int j = 1; j < tps.tuples().size(); j++)
                                        ans += *(float *)cur_values[j];
                                    ans /= tps.tuples().size();
                                    out_tuple.add(ans);
                                    break;
                                }

                                default:
                                    break;
                                    
                            }
                            
                            break;
                        }
                            /*************  case AVG_T  end **************** */

                        default:
                            std::cout<<"NO CASE ENTERED"<<std::endl;
                            break;
                    }

                }
                out_set.set_schema(aggr_schema);
                out_set.add(std::move(out_tuple));
                out_set.print(ss);
            }
          }
          for (SelectExeNode *& tmp_node: select_nodes) {
            delete tmp_node;
          }

  session_event->set_response(ss.str());
  end_trx_if_need(session, trx, true);
  return rc;
}

bool match_table(const Selects &selects, const char *table_name_in_condition, const char *table_name_to_match) {
  if (table_name_in_condition != nullptr) {
    return 0 == strcmp(table_name_in_condition, table_name_to_match);
  }

  return selects.relation_num == 1;
}

static RC schema_add_field(Table *table, const char *field_name, TupleSchema &schema) {
  const FieldMeta *field_meta = table->table_meta().field(field_name);
  if (0 != strcmp("1", field_name) && nullptr == field_meta) {
    LOG_WARN("No such field. %s.%s", table->name(), field_name);
    return RC::SCHEMA_FIELD_MISSING;
  }
  if(0 == strcmp("1", field_name))
      schema.from_table(table, schema);
  else
      schema.add_if_not_exists(field_meta->type(), table->name(), field_meta->name());
  return RC::SUCCESS;
}
// 把所有的表和只跟这张表关联的condition都拿出来，生成最底层的select 执行节点
RC create_selection_executor(Trx *trx, const Selects &selects, const char *db, const char *table_name, SelectExeNode &select_node) {
  // 列出跟这张表关联的Attr
  TupleSchema schema;
  Table * table = DefaultHandler::get_default().find_table(db, table_name);
    if (nullptr == table) {
    LOG_WARN("No such table [%s] in db [%s]", table_name, db);
    return RC::SCHEMA_TABLE_NOT_EXIST;
  }
//  std::cout<<"###"<<selects.attr_num<<std::endl;
  for (int i = selects.attr_num - 1; i >= 0; i--) {
    const RelAttr &attr = selects.attributes[i];
    if (nullptr == attr.relation_name || 0 == strcmp(table_name, attr.relation_name)) {
      if (0 == strcmp("*", attr.attribute_name)) {
        // 列出这张表所有字段
        TupleSchema::from_table(table, schema);
        break; // 没有校验，给出* 之后，再写字段的错误
      }
      else {
        // 列出这张表相关字段
        RC rc = schema_add_field(table, attr.attribute_name, schema);
        if (rc != RC::SUCCESS) {
          return rc;
        }
      }
    }
  }

  // 找出仅与此表相关的过滤条件, 或者都是值的过滤条件
  std::vector<DefaultConditionFilter *> condition_filters;
  for (size_t i = 0; i < selects.condition_num; i++)
  {
    const Condition &condition = selects.conditions[i];
    if ((condition.left_is_attr == 0 && condition.right_is_attr == 0) || // 两边都是值
        (condition.left_is_attr == 1 && condition.right_is_attr == 0 && match_table(selects, condition.left_attr.relation_name, table_name)) ||  // 左边是属性右边是值
        (condition.left_is_attr == 0 && condition.right_is_attr == 1 && match_table(selects, condition.right_attr.relation_name, table_name)) ||  // 左边是值，右边是属性名
        (condition.left_is_attr == 1 && condition.right_is_attr == 1 &&
            match_table(selects, condition.left_attr.relation_name, table_name) && match_table(selects, condition.right_attr.relation_name, table_name)) // 左右都是属性名，并且表名都符合
        )
    {
      DefaultConditionFilter *condition_filter = new DefaultConditionFilter();
      RC rc = condition_filter->init(*table, condition);
      if (rc != RC::SUCCESS)
      {
        delete condition_filter;
        for (DefaultConditionFilter * &filter : condition_filters) {
          delete filter;
        }
        return rc;
      }
      condition_filters.push_back(condition_filter);
    }
  }

  return select_node.init(trx, table, std::move(schema), std::move(condition_filters));
}
