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

#ifndef SSHOPERATION_H_
#define SSHOPERATION_H_

#include <QtCore/QObject>
#include <QtCore/QVariant>

namespace MoleQueue
{

class SshOperation : public QObject
{
  Q_OBJECT
public:
    SshOperation(QObject *parentObject);

signals:
  void complete();
  void error(int errorCode, QString errorString);

public slots:
  virtual bool execute() = 0;
  int errorCode() const;
  QString errorString() const;
  /** \return The merged stdout and stderr of the operation */
  QString output() const;
  /** @return A reference to arbitrary data stored in the command. */
  QVariant & data() {return m_data;}
  /** @return A reference to arbitrary data stored in the command. */
  const QVariant & data() const {return m_data;}
  /** @param newData Arbitrary data to store in the command. */
  void setData(const QVariant &newData) {m_data = newData;}

protected:
  QVariant m_data;
  int m_errorCode;
  QString m_errorString;
  QString m_output;


};

  } /* namespace MoleQueue */
#endif /* SSHOPERATION_H_ */
