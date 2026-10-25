#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QSqlDatabase>

QT_BEGIN_NAMESPACE
namespace Ui {
class MainWindow;
}
QT_END_NAMESPACE

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = nullptr);
    ~MainWindow() override;

private slots:
    void on_addEquipmentButton_clicked();

    void on_clearEquipmentButton_clicked();

    void on_equipmentTableWidget_cellClicked(int row, int column);

    void on_updateEquipmentButton_clicked();

    void on_searchEquipmentButton_clicked();

    void on_showAllEquipmentButton_clicked();

    void on_addBorrowerButton_clicked();

    void on_clearBorrowerButton_clicked();

    void on_borrowerTableWidget_cellClicked(int row, int column);

    void on_updateBorrowerButton_clicked();

    void on_showAllBorrowersButton_clicked();

    void on_searchBorrowerButton_clicked();

    void on_checkoutEquipmentButton_clicked();

    void on_returnEquipmentButton_clicked();

    void on_applyRecordFilterButton_clicked();

    void on_refreshRecordsButton_clicked();

private:
    Ui::MainWindow *ui;
    bool setupDatabase();
    void loadEquipment();
    void loadBorrowers();
    void loadCheckoutOptions();
    void loadRecords();
    QSqlDatabase database;
};
#endif // MAINWINDOW_H
