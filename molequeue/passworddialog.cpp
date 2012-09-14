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

#include "passworddialog.h"
#include "ui_passworddialog.h"

namespace MoleQueue
{

PasswordDialog::PasswordDialog(const QString &hostString,
                               QWidget *parentObject) :
  QDialog(parentObject),
  m_hostString(hostString),
  ui(new Ui::PasswordDialog)
{
  ui->setupUi(this);
}


void PasswordDialog::accept()
{
  emit entered(ui->lineEdit_2->text());

  //QDialog::accept();
}

void PasswordDialog::setHostString(const QString &hostString) {
  m_hostString = hostString;
  ui->hostLabel->setText(m_hostString);
}

void PasswordDialog::setErrorString(const QString &errorString) {
  ui->messageLabel->setText(errorString);
}

void PasswordDialog::clear()
{
  clearPassword();
  ui->hostLabel->setText("");
  ui->messageLabel->setText("");
}

void PasswordDialog::clearPassword()
{
  ui->lineEdit_2->setText("");
}

void PasswordDialog::setPrompt(const QString &prompt)
{
  ui->promptLabel->setText(prompt);
}

} /* namespace MoleQueue */
