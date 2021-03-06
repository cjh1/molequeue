/******************************************************************************

  This source file is part of the MoleQueue project.

  Copyright 2012 Kitware, Inc.

  This source code is released under the New BSD License, (the "License").

  Unless required by applicable law or agreed to in writing, software
  distributed under the License is distributed on an "AS IS" BASIS,
  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
  See the License for the specific language governing permissions and
  limitations under the License.

******************************************************************************/

#include "jsonrpc.h"

#include "job.h"
#include "jobdata.h"
#include "molequeueglobal.h"
#include "qtjson.h"

#include <json/json.h>

#include <QtCore/QDateTime>
#include <QtCore/QDebug>
#include <QtCore/QDir>
#include <QtCore/QHash>
#include <QtCore/QPair>
#include <QtCore/QVariantHash>

namespace MoleQueue
{

JsonRpc::JsonRpc(QObject *parentObject)
  : QObject(parentObject)
{
  qRegisterMetaType<QDir>("QDir");
  qRegisterMetaType<Json::Value>("Json::Value");
  qRegisterMetaType<IdType>("MoleQueue::IdType");
  qRegisterMetaType<JobState>("MoleQueue::JobState");
  qRegisterMetaType<QueueListType>("MoleQueue::QueueListType");
  qRegisterMetaType<JobSubmissionErrorCode>("MoleQueue::JobSubmissionErrorCode");
}

JsonRpc::~JsonRpc()
{
}

PacketType JsonRpc::generateJobRequest(const Job &job,
                                       IdType packetId)
{
  Json::Value packet = generateEmptyRequest(packetId);

  packet["method"] = "submitJob";

  Json::Value paramsObject = QtJson::toJson(job.hash());

  packet["params"] = paramsObject;

  Json::StyledWriter writer;
  std::string ret_stdstr = writer.write(packet);
  PacketType ret (ret_stdstr.c_str());

  registerRequest(packetId, SUBMIT_JOB);

  return ret;
}

PacketType
JsonRpc::generateJobSubmissionConfirmation(IdType moleQueueId,
                                           const QString &workingDirectory,
                                           IdType packetId)
{
  Json::Value packet = generateEmptyResponse(packetId);

  Json::Value resultObject (Json::objectValue);
  resultObject["moleQueueId"] = moleQueueId;
  resultObject["workingDirectory"] = workingDirectory.toStdString();

  packet["result"] = resultObject;

  Json::StyledWriter writer;
  std::string ret_stdstr = writer.write(packet);
  PacketType ret (ret_stdstr.c_str());

  return ret;
}

PacketType JsonRpc::generateErrorResponse(int errorCode,
                                          const QString &message,
                                          IdType packetId)
{
  Json::Value packet = generateEmptyError(packetId);

  packet["error"]["code"]    = errorCode;
  packet["error"]["message"] = message.toStdString();

  Json::StyledWriter writer;
  std::string ret_stdstr = writer.write(packet);
  PacketType ret (ret_stdstr.c_str());

  return ret;
}

PacketType JsonRpc::generateErrorResponse(int errorCode,
                                          const QString &message,
                                          const Json::Value &data,
                                          IdType packetId)
{
  Json::Value packet = generateEmptyError(packetId);

  packet["error"]["code"]    = errorCode;
  packet["error"]["message"] = message.toStdString();
  packet["error"]["data"]    = data;

  Json::StyledWriter writer;
  std::string ret_stdstr = writer.write(packet);
  PacketType ret (ret_stdstr.c_str());

  return ret;
}

PacketType JsonRpc::generateErrorResponse(int errorCode,
                                          const QString &message,
                                          const Json::Value &packetId)
{
  Json::Value packet = generateEmptyError(packetId);

  packet["error"]["code"]    = errorCode;
  packet["error"]["message"] = message.toStdString();

  Json::StyledWriter writer;
  std::string ret_stdstr = writer.write(packet);
  PacketType ret (ret_stdstr.c_str());

  return ret;
}

PacketType JsonRpc::generateErrorResponse(int errorCode,
                                          const QString &message,
                                          const Json::Value &data,
                                          const Json::Value &packetId)
{
  Json::Value packet = generateEmptyError(packetId);

  packet["error"]["code"]    = errorCode;
  packet["error"]["message"] = message.toStdString();
  packet["error"]["data"]    = data;

  Json::StyledWriter writer;
  std::string ret_stdstr = writer.write(packet);
  PacketType ret (ret_stdstr.c_str());

  return ret;
}

PacketType JsonRpc::generateJobCancellation(const Job &job,
                                            IdType packetId)
{
  Json::Value packet = generateEmptyRequest(packetId);

  packet["method"] = "cancelJob";

  Json::Value paramsObject (Json::objectValue);
  paramsObject["moleQueueId"] = job.moleQueueId();

  packet["params"] = paramsObject;

  Json::StyledWriter writer;
  std::string ret_stdstr = writer.write(packet);
  PacketType ret (ret_stdstr.c_str());

  registerRequest(packetId, CANCEL_JOB);

  return ret;
}

PacketType JsonRpc::generateJobCancellationConfirmation(IdType moleQueueId,
                                                        IdType packetId)
{
  Json::Value packet = generateEmptyResponse(packetId);

  packet["result"] = moleQueueId;

  Json::StyledWriter writer;
  std::string ret_stdstr = writer.write(packet);
  PacketType ret (ret_stdstr.c_str());

  return ret;
}

PacketType JsonRpc::generateLookupJobRequest(IdType moleQueueId,
                                             IdType packetId)
{
  Json::Value packet = generateEmptyRequest(packetId);

  packet["method"] = "lookupJob";

  Json::Value paramsObject(Json::objectValue);
  paramsObject["moleQueueId"] = moleQueueId;

  packet["params"] = paramsObject;

  Json::StyledWriter writer;
  std::string ret_stdstr = writer.write(packet);
  PacketType ret(ret_stdstr.c_str());

  registerRequest(packetId, LOOKUP_JOB);

  return ret;
}

PacketType JsonRpc::generateLookupJobResponse(const Job &req,
                                              IdType moleQueueId,
                                              IdType packetId)
{
  Json::Value packet;

  if (!req.isValid()) {
    packet = generateEmptyError(packetId);
    Json::Value errorObject(Json::objectValue);
    errorObject["message"] = "Unknown MoleQueue ID";
    errorObject["code"] = 0;
    errorObject["data"] = moleQueueId;
    packet["error"] = errorObject;
  }
  else {
    packet = generateEmptyResponse(packetId);
    packet["result"] = QtJson::toJson(req.hash());
  }

  Json::StyledWriter writer;
  std::string ret_stdstr = writer.write(packet);
  PacketType ret(ret_stdstr.c_str());

  return ret;
}

PacketType JsonRpc::generateQueueListRequest(IdType packetId)
{
  Json::Value packet = generateEmptyRequest(packetId);

  packet["method"] = "listQueues";

  Json::StyledWriter writer;
  std::string ret_stdstr = writer.write(packet);
  PacketType ret (ret_stdstr.c_str());

  registerRequest(packetId, LIST_QUEUES);

  return ret;
}

PacketType JsonRpc::generateQueueList(const QueueListType &queueList,
                                      IdType packetId)
{
  Json::Value packet = generateEmptyResponse(packetId);

  Json::Value resultObject (Json::objectValue);
  foreach (const QString queueName, queueList.keys()) {

    Json::Value programArray (Json::arrayValue);
    foreach (const QString prog, queueList[queueName]) {
      const std::string progName = prog.toStdString();
      programArray.append(progName);
    }
    resultObject[queueName.toStdString()] = programArray;
  }

  packet["result"] = resultObject;

  Json::StyledWriter writer;
  std::string ret_stdstr = writer.write(packet);
  PacketType ret (ret_stdstr.c_str());

  return ret;
}

PacketType
JsonRpc::generateJobStateChangeNotification(IdType moleQueueId,
                                            JobState oldState,
                                            JobState newState)
{
  Json::Value packet = generateEmptyNotification();

  packet["method"] = "jobStateChanged";

  Json::Value paramsObject (Json::objectValue);
  paramsObject["moleQueueId"]         = moleQueueId;
  paramsObject["oldState"]            = jobStateToString(oldState);
  paramsObject["newState"]            = jobStateToString(newState);

  packet["params"] = paramsObject;

  Json::StyledWriter writer;
  std::string ret_stdstr = writer.write(packet);
  PacketType ret (ret_stdstr.c_str());

  return ret;
}

void JsonRpc::interpretIncomingPacket(Connection* connection,
                                      const Message msg)
{
  // Read packet into a Json value
  Json::Reader reader;
  Json::Value root;

  if (!reader.parse(msg.data().constData(),
                    msg.data().constData() + msg.data().size(),
                    root, false)) {
    handleUnparsablePacket(connection, msg);
    return;
  }

  // Submit the root node for processing
  interpretIncomingJsonRpc(connection, msg.replyTo(), root);
}

void JsonRpc::interpretIncomingJsonRpc(Connection *connection,
                                       EndpointId replyTo,
                                       const Json::Value &data)
{
  // Handle batch requests recursively:
  if (data.isArray()) {
    for (Json::Value::const_iterator it = data.begin(), it_end = data.end();
         it != it_end; ++it) {
      interpretIncomingJsonRpc(connection, replyTo, *it);
    }

    return;
  }

  if (!data.isObject()) {
    handleInvalidRequest(connection, replyTo, data);
    return;
  }

  PacketForm   form   = guessPacketForm(data);
  PacketMethod method = guessPacketMethod(data);

  // Validate detected type
  switch (form) {
  case REQUEST_PACKET:
    if (!validateRequest(data, false))
      form = INVALID_PACKET;
    break;
  case RESULT_PACKET:
  case ERROR_PACKET:
    if (!validateResponse(data, false))
      form = INVALID_PACKET;
    break;
  case NOTIFICATION_PACKET:
    if (!validateNotification(data, false))
      form = INVALID_PACKET;
    break;
  default:
  case INVALID_PACKET:
    break;
  }

  switch (method) {
  case IGNORE_METHOD:
    break;
  default:
  case INVALID_METHOD:
    handleInvalidRequest(connection, replyTo, data);
    break;
  case UNRECOGNIZED_METHOD:
    handleUnrecognizedRequest(connection, replyTo, data);
    break;
  case LIST_QUEUES:
  {
    switch (form) {
    default:
    case INVALID_PACKET:
    case NOTIFICATION_PACKET:
      handleInvalidRequest(connection, replyTo, data);
      break;
    case REQUEST_PACKET:
      handleListQueuesRequest(connection, replyTo, data);
      break;
    case RESULT_PACKET:
      handleListQueuesResult(data);
      break;
    case ERROR_PACKET:
      handleListQueuesError(connection, replyTo, data);
      break;
    }
    break;
  }
  case SUBMIT_JOB:
  {
    switch (form) {
    default:
    case INVALID_PACKET:
    case NOTIFICATION_PACKET:
      handleInvalidRequest(connection, replyTo, data);
      break;
    case REQUEST_PACKET:
      handleSubmitJobRequest(connection, replyTo, data);
      break;
    case RESULT_PACKET:
      handleSubmitJobResult(data);
      break;
    case ERROR_PACKET:
      handleSubmitJobError(data);
      break;
    }
    break;
  }
  case CANCEL_JOB:
  {
    switch (form) {
    default:
    case INVALID_PACKET:
    case NOTIFICATION_PACKET:
      handleInvalidRequest(connection, replyTo, data);
      break;
    case REQUEST_PACKET:
      handleCancelJobRequest(connection, replyTo, data);
      break;
    case RESULT_PACKET:
      handleCancelJobResult(data);
      break;
    case ERROR_PACKET:
      handleCancelJobError(data);
      break;
    }
    break;
  }
  case LOOKUP_JOB:
  {
    switch (form) {
    default:
    case INVALID_PACKET:
    case NOTIFICATION_PACKET:
      handleInvalidRequest(connection, replyTo, data);
      break;
    case REQUEST_PACKET:
      handleLookupJobRequest(connection, replyTo, data);
      break;
    case RESULT_PACKET:
      handleLookupJobResult(data);
      break;
    case ERROR_PACKET:
      handleLookupJobError(data);
      break;
    }
    break;
  }
  case JOB_STATE_CHANGED:
  {
    switch (form) {
    default:
    case INVALID_PACKET:
    case REQUEST_PACKET:
    case RESULT_PACKET:
    case ERROR_PACKET:
      handleInvalidRequest(connection, replyTo, data);
      break;
    case NOTIFICATION_PACKET:
      handleJobStateChangedNotification(data);
      break;
    }
    break;
  }
  }

  // Remove responses from pendingRequests lookup table
  // id is guaranteed to exist after earlier validation
  if (form == RESULT_PACKET || form == ERROR_PACKET)
    registerReply(static_cast<IdType>(data["id"].asLargestUInt()));

}

bool JsonRpc::validateRequest(const PacketType &packet, bool strict)
{
  Json::Value root;
  Json::Reader reader;
  reader.parse(packet.constData(), packet.constData() + packet.size(),
               root, false);
  return validateRequest(root, strict);
}

bool JsonRpc::validateRequest(const Json::Value &packet, bool strict)
{
  if (!packet.isObject())
    return false;

  Json::Value::Members members = packet.getMemberNames();

  // Check that the required members are present
  bool found_jsonrpc = false;
  bool found_method = false;
  bool found_params = false;
  bool found_id = false;
  Json::Value::Members extraMembers;

  for (Json::Value::Members::const_iterator it = members.begin(),
       it_end = members.end(); it != it_end; ++it) {
    if (!found_jsonrpc && it->compare("jsonrpc") == 0) {
      found_jsonrpc = true;
    }
    else if (!found_method && it->compare("method") == 0) {
      found_method = true;
    }
    else if (!found_params && it->compare("params") == 0) {
      found_params = true;
    }
    else if (!found_id && it->compare("id") == 0) {
      found_id = true;
    }
    else {
      extraMembers.push_back(*it);
    }
  }

  if (!found_jsonrpc && strict)
      return false;

  if (!found_method)
    return false;

  // Params are optional.
  //  if (!found_params)
  //    return false;

  if (!found_id)
    return false;

  // Validate objects
  // "method" must be a string
  if (!packet["method"].isString())
    return false;

  // "params" may be omitted, but must be structured if present
  if (found_params &&
      !packet["params"].isObject() && !packet["params"].isArray()) {
      return false;
  }

  // "id" must be a string, a number, or null, but should not be null or
  // fractional
  const Json::Value & idValue = packet["id"];
  if (!idValue.isString() && !idValue.isNumeric() && !idValue.isNull())
    return false;

  // Print extra members
  if (extraMembers.size() != 0 && strict)
    return false;

  return true;
}

bool JsonRpc::validateResponse(const PacketType &packet, bool strict)
{
  Json::Value root;
  Json::Reader reader;
  reader.parse(packet.constData(), packet.constData() + packet.size(),
               root, false);
  return validateResponse(root, strict);
}

bool JsonRpc::validateResponse(const Json::Value &packet, bool strict)
{
  if (!packet.isObject())
    return false;

  Json::Value::Members members = packet.getMemberNames();

  // Check that the required members are present
  bool found_jsonrpc = false;
  bool found_result = false;
  bool found_error = false;
  bool found_id = false;
  Json::Value::Members extraMembers;

  for (Json::Value::Members::const_iterator it = members.begin(),
       it_end = members.end(); it != it_end; ++it) {
    if (!found_jsonrpc && it->compare("jsonrpc") == 0) {
      found_jsonrpc = true;
    }
    else if (!found_result && it->compare("result") == 0) {
      found_result = true;
    }
    else if (!found_error && it->compare("error") == 0) {
      found_error = true;
    }
    else if (!found_id && it->compare("id") == 0) {
      found_id = true;
    }
    else {
      extraMembers.push_back(*it);
    }
  }

  if (!found_jsonrpc && strict)
    return false;

  if (!found_result && !found_error)
    return false;

  if (found_result && found_error)
    return false;

  if (!found_id)
    return false;

  // Validate error object if present
  if (found_error) {
    const Json::Value & errorObject = packet["error"];
    if (!errorObject.isObject())
      return false;

    // "code" must be an integer
    if (!errorObject["code"].isIntegral())
      return false;

    // "message" must be a string
    if (!errorObject["message"].isString())
      return false;
  }

  // "id" must be a string, a number, or null, but should not be null or
  // fractional
  const Json::Value & idValue = packet["id"];
  if (!idValue.isString() && !idValue.isNumeric() && !idValue.isNull())
    return false;

  // Print extra members
  if (extraMembers.size() != 0 && strict)
    return false;

  return true;
}

bool JsonRpc::validateNotification(const PacketType &packet, bool strict)
{
  Json::Value root;
  Json::Reader reader;
  reader.parse(packet.constData(), packet.constData() + packet.size(),
               root, false);
  return validateNotification(root, strict);
}

bool JsonRpc::validateNotification(const Json::Value &packet, bool strict)
{
  if (!packet.isObject())
    return false;

  Json::Value::Members members = packet.getMemberNames();

  // Check that the required members are present
  bool found_jsonrpc = false;
  bool found_method = false;
  bool found_params = false;
  bool found_id = false;
  Json::Value::Members extraMembers;

  for (Json::Value::Members::const_iterator it = members.begin(),
       it_end = members.end(); it != it_end; ++it) {
    if (!found_jsonrpc && it->compare("jsonrpc") == 0) {
      found_jsonrpc = true;
    }
    else if (!found_method && it->compare("method") == 0) {
      found_method = true;
    }
    else if (!found_params && it->compare("params") == 0) {
      found_params = true;
    }
    else if (!found_id && it->compare("id") == 0) {
      found_id = true;
    }
    else {
      extraMembers.push_back(*it);
    }
  }

  if (!found_jsonrpc && strict)
      return false;

  if (!found_method)
    return false;

  // Params are optional.
  //  if (!found_params)
  //    return false;

  if (found_id)
    return false;

  // Validate objects
  // "method" must be a string
  if (!packet["method"].isString())
    return false;

  // "params" may be omitted, but must be structured if present
  if (found_params &&
      !packet["params"].isObject() && !packet["params"].isArray()) {
    return false;
  }

  // Print extra members
  if (extraMembers.size() != 0 && strict)
    return false;

  return true;
}

Json::Value JsonRpc::generateEmptyRequest(IdType id)
{
  Json::Value ret (Json::objectValue);
  ret["jsonrpc"] = "2.0";
  ret["method"] = Json::nullValue;
  ret["id"] = id;

  return ret;
}

Json::Value JsonRpc::generateEmptyResponse(IdType id)
{
  Json::Value ret (Json::objectValue);
  ret["jsonrpc"] = "2.0";
  ret["result"] = Json::nullValue;
  ret["id"] = id;

  return ret;
}

Json::Value JsonRpc::generateEmptyError(IdType id)
{
  Json::Value ret (Json::objectValue);
  ret["jsonrpc"] = "2.0";
  Json::Value errorValue (Json::objectValue);
  errorValue["code"] = Json::nullValue;
  errorValue["message"] = Json::nullValue;
  ret["error"] = Json::nullValue;
  ret["id"] = id;

  return ret;
}

Json::Value JsonRpc::generateEmptyError(const Json::Value &id)
{
  Json::Value ret (Json::objectValue);
  ret["jsonrpc"] = "2.0";
  Json::Value errorValue (Json::objectValue);
  errorValue["code"] = Json::nullValue;
  errorValue["message"] = Json::nullValue;
  ret["error"] = Json::nullValue;
  ret["id"] = id;

  return ret;
}

Json::Value JsonRpc::generateEmptyNotification()
{
  Json::Value ret (Json::objectValue);
  ret["jsonrpc"] = "2.0";
  ret["method"] = Json::nullValue;

  return ret;
}

JsonRpc::PacketForm JsonRpc::guessPacketForm(const Json::Value &root) const
{
  if (!root.isObject())
    return INVALID_PACKET;

  if (root["method"] != Json::nullValue) {
    if (root["id"] != Json::nullValue)
      return REQUEST_PACKET;
    else
      return NOTIFICATION_PACKET;
  }
  else if (root["result"] != Json::nullValue)
    return RESULT_PACKET;
  else if (root["error"] != Json::nullValue)
    return ERROR_PACKET;

  return INVALID_PACKET;
}

JsonRpc::PacketMethod JsonRpc::guessPacketMethod(const Json::Value &root) const
{
  if (!root.isObject())
    return INVALID_METHOD;

  const Json::Value & methodValue = root["method"];

  if (methodValue != Json::nullValue) {
    if (!methodValue.isString())
      return INVALID_METHOD;

    const char *methodCString = methodValue.asCString();

    if (qstrcmp(methodCString, "listQueues") == 0)
      return LIST_QUEUES;
    else if (qstrcmp(methodCString, "submitJob") == 0)
      return SUBMIT_JOB;
    else if (qstrcmp(methodCString, "cancelJob") == 0)
      return CANCEL_JOB;
    else if (qstrcmp(methodCString, "lookupJob") == 0)
      return LOOKUP_JOB;
    else if (qstrcmp(methodCString, "jobStateChanged") == 0)
      return JOB_STATE_CHANGED;

    return UNRECOGNIZED_METHOD;
  }

  // No method present -- this is a reply. Determine if it's a reply to this
  // client.
  const Json::Value & idValue = root["id"];

  if (idValue != Json::nullValue) {
    // We will only submit unsigned integral ids
    if (!idValue.isIntegral())
      return IGNORE_METHOD;

    IdType packetId = static_cast<IdType>(idValue.asLargestUInt());

    if (m_pendingRequests.contains(packetId)) {
      return m_pendingRequests[packetId];
    }

    // if the packetId isn't in the lookup table, it's not from us
    return IGNORE_METHOD;
  }

  // No method or id present?
  return INVALID_METHOD;
}

void JsonRpc::handleUnparsablePacket(Connection *connection,
                                     const Message msg) const
{
  Json::Value errorData (Json::objectValue);

  errorData["receivedPacket"] = msg.data().constData();

  emit invalidPacketReceived(connection, msg.replyTo(), Json::nullValue,
                             errorData);
}

void JsonRpc::handleInvalidRequest(Connection *connection,
                                   EndpointId replyTo,
                                   const Json::Value &root) const
{
  Json::Value errorData (Json::objectValue);

  errorData["receivedJson"] = Json::Value(root);

  emit invalidRequestReceived(connection, replyTo,
                              (root.isObject()) ? root["id"] : Json::nullValue,
                              errorData);
}

void JsonRpc::handleUnrecognizedRequest(Connection *connection,
                                        EndpointId replyTo,
                                        const Json::Value &root) const
{
  Json::Value errorData (Json::objectValue);

  errorData["receivedJson"] = root;

  emit unrecognizedRequestReceived(connection, replyTo, root["id"], errorData);
}

void JsonRpc::handleListQueuesRequest(Connection *connection,
                                      EndpointId replyTo,
                                      const Json::Value &root) const
{
  const IdType id = static_cast<IdType>(root["id"].asLargestUInt());
  emit queueListRequestReceived(connection, replyTo, id);
}

void JsonRpc::handleListQueuesResult(const Json::Value &root) const
{
  const IdType id = static_cast<IdType>(root["id"].asLargestUInt());

  const Json::Value &resultObject = root["result"];
  if (!resultObject.isObject()) {
    Json::StyledWriter writer;
    const std::string requestString = writer.write(root);
    qWarning() << "Error: Queue list result is ill-formed:\n"
               << requestString.c_str();
    return;
  }

  // Populate queue list:
  QueueListType queueList;
  queueList.reserve(resultObject.size());

  // Iterate through queues
  for (Json::Value::const_iterator it = resultObject.begin(),
       it_end = resultObject.end(); it != it_end; ++it) {
    const QString queueName (it.memberName());

    // Extract program data
    const Json::Value &programArray = *it;

    // No programs, just add an empty list
    if (programArray.isNull()) {
      queueList.insert(queueName, QStringList());
      continue;
    }

    // Not an array? Add an empty list
    if (!programArray.isArray()) {
      Json::StyledWriter writer;
      const std::string programsString = writer.write(programArray);
      qWarning() << "Error: List of programs for" << queueName
                 << "is ill-formed:\n"
                 << programsString.c_str();
      queueList.insert(queueName, QStringList());
      continue;
    }

    QStringList programList;
#if QT_VERSION >= QT_VERSION_CHECK(4,7,0)
    programList.reserve(programArray.size());
#endif
    // Iterate through programs
    for (Json::Value::const_iterator pit = programArray.begin(),
         pit_end = programArray.end(); pit != pit_end; ++pit) {
      if ((*pit).isString())
        programList << (*pit).asCString();
    }

    queueList.insert(queueName, programList);
  }


  emit queueListReceived(id, queueList);
}

void JsonRpc::handleListQueuesError(Connection *connection,
                                    EndpointId replyTo,
                                    const Json::Value &root) const
{
  Q_UNUSED(connection);
  Q_UNUSED(replyTo);
  Q_UNUSED(root);
  qWarning() << Q_FUNC_INFO << "is not implemented.";
}

void JsonRpc::handleSubmitJobRequest(Connection *connection,
                                     EndpointId replyTo,
                                     const Json::Value &root) const
{
  const IdType id = static_cast<IdType>(root["id"].asLargestUInt());

  const Json::Value &paramsObject = root["params"];
  if (!paramsObject.isObject()) {
    Json::StyledWriter writer;
    const std::string requestString = writer.write(root);
    qWarning() << "Error: submitJob request is ill-formed:\n"
               << requestString.c_str();
    return;
  }

  // Populate options object:
  QVariantHash optionHash = QtJson::toVariant(paramsObject).toHash();

  emit jobSubmissionRequestReceived(connection, replyTo, id, optionHash);
}

void JsonRpc::handleSubmitJobResult(const Json::Value &root) const
{
  const IdType id = static_cast<IdType>(root["id"].asLargestUInt());

  const Json::Value &resultObject = root["result"];

  IdType moleQueueId;
  QDir workingDirectory;

  if (!resultObject["moleQueueId"].isIntegral() ||
      !resultObject["workingDirectory"].isString()) {
    Json::StyledWriter writer;
    const std::string responseString = writer.write(root);
    qWarning() << "Job submission result is ill-formed:\n"
               << responseString.c_str();
    return;
  }

  moleQueueId = static_cast<IdType>(
        resultObject["moleQueueId"].asLargestUInt());
  workingDirectory = QDir(QString(
                            resultObject["workingDirectory"].asCString()));

  emit successfulSubmissionReceived(id, moleQueueId, workingDirectory);
}

void JsonRpc::handleSubmitJobError(const Json::Value &root) const
{
  const IdType id = static_cast<IdType>(root["id"].asLargestUInt());

  if (!root["error"]["code"].isIntegral() ||
      !root["error"]["message"].isString()) {
    Json::StyledWriter writer;
    const std::string responseString = writer.write(root);
    qWarning() << "Job submission failure response is ill-formed:\n"
               << responseString.c_str();
    return;
  }

  const JobSubmissionErrorCode errorCode = static_cast<JobSubmissionErrorCode>(
        root["error"]["code"].asLargestInt());

  const QString errorMessage (root["error"]["message"].asCString());

  emit failedSubmissionReceived(id, errorCode, errorMessage);
}

void JsonRpc::handleCancelJobRequest(MoleQueue::Connection *connection,
                                     const EndpointId replyTo,
                                     const Json::Value &root) const
{
  const IdType id = static_cast<IdType>(root["id"].asLargestUInt());

  const Json::Value &paramsObject = root["params"];

  if (!paramsObject.isObject() ||
      !paramsObject["moleQueueId"].isIntegral()) {
    Json::StyledWriter writer;
    const std::string responseString = writer.write(root);
    qWarning() << "Job cancellation request is ill-formed:\n"
               << responseString.c_str();
    return;
  }

  const IdType moleQueueId = static_cast<IdType>(
        paramsObject["moleQueueId"].asLargestUInt());

  emit jobCancellationRequestReceived(connection, replyTo, id, moleQueueId);
}

void JsonRpc::handleCancelJobResult(const Json::Value &root) const
{
  const IdType id = static_cast<IdType>(root["id"].asLargestUInt());

  const Json::Value &resultObject = root["result"];

  IdType moleQueueId;

  if (!resultObject.isIntegral()) {
    Json::StyledWriter writer;
    const std::string responseString = writer.write(root);
    qWarning() << "Job cancellation result is ill-formed:\n"
               << responseString.c_str();
    return;
  }

  moleQueueId = static_cast<IdType>(resultObject.asLargestUInt());

  emit jobCancellationConfirmationReceived(id, moleQueueId);
}

void JsonRpc::handleCancelJobError(const Json::Value &root) const
{
  Q_UNUSED(root);
  qWarning() << Q_FUNC_INFO << "is not implemented.";
}

void JsonRpc::handleLookupJobRequest(Connection *connection,
                                     const EndpointId replyTo,
                                     const Json::Value &root) const
{
  const IdType id = static_cast<IdType>(root["id"].asLargestUInt());

  const Json::Value &paramsObject = root["params"];

  if (!paramsObject.isObject() ||
      !paramsObject["moleQueueId"].isIntegral()) {
    Json::StyledWriter writer;
    const std::string responseString = writer.write(root);
    qWarning() << "Job lookup request is ill-formed:\n"
               << responseString.c_str();
    return;
  }

  const IdType moleQueueId = static_cast<IdType>(
        paramsObject["moleQueueId"].asLargestUInt());

  emit lookupJobRequestReceived(connection, replyTo, id, moleQueueId);
}

void JsonRpc::handleLookupJobResult(const Json::Value &root) const
{
  const IdType id = static_cast<IdType>(root["id"].asLargestUInt());

  const Json::Value &resultObject = root["result"];

  if (!resultObject.isObject()) {
    Json::StyledWriter writer;
    const std::string responseString = writer.write(root);
    qWarning() << "Job lookup result is ill-formed:\n"
               << responseString.c_str();
    return;
  }

  QVariantHash hash = QtJson::toVariant(resultObject).toHash();

  emit lookupJobResponseReceived(id, hash);
}

void JsonRpc::handleLookupJobError(const Json::Value &root) const
{
  const IdType id = static_cast<IdType>(root["id"].asLargestUInt());

  if (!root["error"]["code"].isIntegral() ||
      !root["error"]["data"].isIntegral() ||
      !root["error"]["message"].isString()) {
    Json::StyledWriter writer;
    const std::string responseString = writer.write(root);
    qWarning() << "Job lookup failure response is ill-formed:\n"
               << responseString.c_str();
    return;
  }

  const IdType moleQueueId =
      static_cast<IdType>(root["error"]["data"].asLargestUInt());

  emit lookupJobErrorReceived(id, moleQueueId);
}

void JsonRpc::handleJobStateChangedNotification(const Json::Value &root) const
{
  const Json::Value &paramsObject = root["params"];

  IdType moleQueueId;
  JobState oldState;
  JobState newState;

  if (!paramsObject.isObject() ||
      !paramsObject["moleQueueId"].isIntegral() ||
      !paramsObject["oldState"].isString() ||
      !paramsObject["newState"].isString() ){
    Json::StyledWriter writer;
    const std::string requestString = writer.write(root);
    qWarning() << "Job cancellation result is ill-formed:\n"
               << requestString.c_str();
    return;
  }

  moleQueueId = static_cast<IdType>(
        paramsObject["moleQueueId"].asLargestUInt());
  oldState = stringToJobState(paramsObject["oldState"].asCString());
  newState = stringToJobState(paramsObject["newState"].asCString());

  emit jobStateChangeReceived(moleQueueId, oldState, newState);
}

void JsonRpc::registerRequest(IdType packetId,
                              JsonRpc::PacketMethod method)
{
  m_pendingRequests[packetId] = method;
}

void JsonRpc::registerReply(IdType packetId)
{
  m_pendingRequests.remove(packetId);
}

} // end namespace MoleQueue
