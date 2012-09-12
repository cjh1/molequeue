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

#include "askpassword.h"
#include "passworddialog.h"

namespace MoleQueue
  {

AskPassword::AskPassword(QObject *parentObject) :
  QObject(parentObject)
{

}

QDialogAskPassword::QDialogAskPassword(QObject *parentObject) :
  AskPassword(parentObject), m_passwordDialog(NULL)
{

}

QDialogAskPassword::~QDialogAskPassword()
{
  delete m_passwordDialog;
}

void QDialogAskPassword::ask(const QString &hostString)
{
  if(m_passwordDialog == NULL) {
    m_passwordDialog = new PasswordDialog(QString("test@salix"));
    connect(m_passwordDialog, SIGNAL(entered(const QString&)),
            this, SIGNAL(entered(const QString&)));
  }

  m_passwordDialog->setHostString(hostString);
  m_passwordDialog->show();
  m_passwordDialog->raise();
}

void QDialogAskPassword::incorrect()
{
  m_passwordDialog->setErrorString("Incorrect password");
  m_passwordDialog->clearPassword();
  m_passwordDialog->setVisible(true);
  m_passwordDialog->show();
  m_passwordDialog->raise();
}

void QDialogAskPassword::correct()
{
  m_passwordDialog->clear();
  m_passwordDialog->hide();
  delete m_passwordDialog;
  m_passwordDialog = NULL;
}

} /* namespace MoleQueue */
