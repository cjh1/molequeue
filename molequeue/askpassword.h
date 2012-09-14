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

#ifndef ASKPASSWORD_H_
#define ASKPASSWORD_H_

#include <qobject.h>

namespace MoleQueue
{

class PasswordDialog;

class AskPassword: public QObject
{
  Q_OBJECT
public:
  AskPassword(QObject *parentObject);

public slots:
  virtual void ask(const QString &hostString,
                   const QString &prompt) = 0;
  virtual void incorrect() = 0;
  virtual void correct() = 0;

signals:
  void entered(const QString &password);
};


class QDialogAskPassword : public AskPassword
{
  Q_OBJECT
public:
  QDialogAskPassword(QObject *parentObject);
  ~QDialogAskPassword();

public slots:
  void ask(const QString &hostString,
           const QString &prompt);
  void incorrect();
  void correct();
  void setPrompt(const QString &prompt);

private:
  PasswordDialog *m_passwordDialog;
};

} /* namespace MoleQueue */

#endif /* ASKPASSWORD_H_ */
