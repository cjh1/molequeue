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

#ifndef PASSWORDWIDGET_H_
#define PASSWORDWIDGET_H_

#include <QtGui/QDialog>
#include <QtGui/QAbstractButton>

namespace Ui {
class PasswordDialog;
}


namespace MoleQueue
{

class PasswordDialog: public QDialog
{
  Q_OBJECT
public:
  PasswordDialog(const QString &hostString, QWidget *parent = 0);
  void setHostString(const QString &hostString);
  void setErrorString(const QString &errorString);
  void setPrompt(const QString &prompt);

public slots:
  void accept();
  void clear();
  void clearPassword();


signals:
  void entered(const QString &password);

private:
  QString m_hostString;
  Ui::PasswordDialog *ui;

};

} /* namespace MoleQueue */

#endif /* PASSWORDWIDGET_H_ */
