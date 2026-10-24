#include "mainwindow.h"
#include "ui_mainwindow.h"

#include <QMessageBox>
#include <QSqlError>
#include <QSqlQuery>
#include <QStatusBar>
#include <QStringList>
#include <QTableWidgetItem>
#include <QHeaderView>

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
{
    ui->setupUi(this);

    ui->equipmentTableWidget
        ->horizontalHeader()
        ->setSectionResizeMode(QHeaderView::Stretch);

    ui->borrowerTableWidget
        ->horizontalHeader()
        ->setSectionResizeMode(QHeaderView::Stretch);

    ui->recordsTableWidget
        ->horizontalHeader()
        ->setSectionResizeMode(QHeaderView::Stretch);

    if (setupDatabase()) {
        statusBar()->showMessage("Database connected successfully.");
        loadEquipment();
        loadBorrowers();
    }
}

MainWindow::~MainWindow()
{
    delete ui;
}

bool MainWindow::setupDatabase()
{
    database = QSqlDatabase::addDatabase("QSQLITE");
    database.setDatabaseName("techtrack.db");

    if (!database.open()) {
        QMessageBox::critical(
            this,
            "Database Error",
            "The database could not be opened:\n" +
                database.lastError().text()
            );
        return false;
    }

    QSqlQuery query(database);

    if (!query.exec("PRAGMA foreign_keys = ON")) {
        QMessageBox::critical(
            this,
            "Database Error",
            query.lastError().text()
            );
        return false;
    }

    QStringList tableCommands = {
        R"(
            CREATE TABLE IF NOT EXISTS Equipment (
                equipment_id INTEGER PRIMARY KEY AUTOINCREMENT,
                name TEXT NOT NULL,
                category TEXT,
                condition TEXT NOT NULL DEFAULT 'Good',
                serial_number TEXT UNIQUE,
                status TEXT NOT NULL DEFAULT 'Available'
            )
        )",

        R"(
            CREATE TABLE IF NOT EXISTS Borrowers (
                borrower_id INTEGER PRIMARY KEY AUTOINCREMENT,
                name TEXT NOT NULL,
                email TEXT,
                phone TEXT
            )
        )",

        R"(
            CREATE TABLE IF NOT EXISTS Checkouts (
                checkout_id INTEGER PRIMARY KEY AUTOINCREMENT,
                equipment_id INTEGER NOT NULL,
                borrower_id INTEGER NOT NULL,
                checkout_date TEXT NOT NULL,
                due_date TEXT NOT NULL,
                return_date TEXT,
                status TEXT NOT NULL DEFAULT 'Checked Out',
                FOREIGN KEY (equipment_id)
                    REFERENCES Equipment(equipment_id),
                FOREIGN KEY (borrower_id)
                    REFERENCES Borrowers(borrower_id)
            )
        )"
    };

    for (const QString &command : tableCommands) {
        if (!query.exec(command)) {
            QMessageBox::critical(
                this,
                "Database Error",
                "A database table could not be created:\n" +
                    query.lastError().text()
                );
            return false;
        }
    }

    // Add the condition column to an older database if it is missing.
    bool conditionColumnExists = false;
    QSqlQuery columnQuery(database);

    if (columnQuery.exec("PRAGMA table_info(Equipment)")) {
        while (columnQuery.next()) {
            if (columnQuery.value(1).toString() == "condition") {
                conditionColumnExists = true;
                break;
            }
        }
    }

    if (!conditionColumnExists) {
        if (!query.exec(
                "ALTER TABLE Equipment "
                "ADD COLUMN condition TEXT NOT NULL DEFAULT 'Good'"
                )) {
            QMessageBox::critical(
                this,
                "Database Error",
                "The Equipment table could not be updated:\n" +
                    query.lastError().text()
                );
            return false;
        }
    }

    return true;
}

void MainWindow::loadBorrowers()
{
    ui->borrowerTableWidget->setRowCount(0);

    QSqlQuery query(database);

    if (!query.exec(
            "SELECT borrower_id, name, email, phone "
            "FROM Borrowers "
            "ORDER BY borrower_id"
            )) {
        QMessageBox::critical(
            this,
            "Borrower Error",
            "The borrower list could not be loaded:\n" +
                query.lastError().text()
            );
        return;
    }

    while (query.next()) {
        int row = ui->borrowerTableWidget->rowCount();
        ui->borrowerTableWidget->insertRow(row);

        for (int column = 0; column < 4; column++) {
            ui->borrowerTableWidget->setItem(
                row,
                column,
                new QTableWidgetItem(
                    query.value(column).toString()
                    )
                );
        }
    }
}

void MainWindow::loadEquipment()
{
    ui->equipmentTableWidget->setRowCount(0);

    QSqlQuery query(database);

    if (!query.exec(
            "SELECT equipment_id, serial_number, name, category, "
            "condition, status "
            "FROM Equipment "
            "ORDER BY equipment_id"
            )) {
        QMessageBox::critical(
            this,
            "Equipment Error",
            "The equipment list could not be loaded:\n" +
                query.lastError().text()
            );
        return;
    }

    while (query.next()) {
        int row = ui->equipmentTableWidget->rowCount();
        ui->equipmentTableWidget->insertRow(row);

        for (int column = 0; column < 6; column++) {
            ui->equipmentTableWidget->setItem(
                row,
                column,
                new QTableWidgetItem(
                    query.value(column).toString()
                    )
                );
        }
    }
}

void MainWindow::on_addEquipmentButton_clicked()
{
    QString assetTag = ui->assetTagLineEdit->text().trimmed();
    QString equipmentName =
        ui->equipmentNameLineEdit->text().trimmed();
    QString category = ui->categoryComboBox->currentText();
    QString condition = ui->conditionComboBox->currentText();

    // Make sure the required fields are not empty.
    if (assetTag.isEmpty() || equipmentName.isEmpty()) {
        QMessageBox::warning(
            this,
            "Missing Information",
            "Please enter an asset tag and equipment name."
            );
        return;
    }

    QSqlQuery query(database);

    query.prepare(
        "INSERT INTO Equipment "
        "(serial_number, name, category, condition, status) "
        "VALUES (:assetTag, :name, :category, :condition, 'Available')"
        );

    query.bindValue(":assetTag", assetTag);
    query.bindValue(":name", equipmentName);
    query.bindValue(":category", category);
    query.bindValue(":condition", condition);

    if (!query.exec()) {
        QMessageBox::critical(
            this,
            "Equipment Error",
            "The equipment could not be added.\n\n"
            "Make sure the asset tag is not already being used.\n\n" +
                query.lastError().text()
            );
        return;
    }

    QMessageBox::information(
        this,
        "Equipment Added",
        "The equipment was added successfully."
        );

    // Clear the form for the next equipment item.
    ui->assetTagLineEdit->clear();
    ui->equipmentNameLineEdit->clear();
    ui->categoryComboBox->setCurrentIndex(0);
    ui->conditionComboBox->setCurrentIndex(0);

    loadEquipment();
}

void MainWindow::on_clearEquipmentButton_clicked()
{
    ui->assetTagLineEdit->clear();
    ui->equipmentNameLineEdit->clear();
    ui->categoryComboBox->setCurrentIndex(0);
    ui->conditionComboBox->setCurrentIndex(0);
    ui->equipmentTableWidget->clearSelection();
    ui->assetTagLineEdit->setFocus();
}

void MainWindow::on_equipmentTableWidget_cellClicked(
    int row,
    int column
    )
{
    Q_UNUSED(column);

    QTableWidgetItem *assetTagItem =
        ui->equipmentTableWidget->item(row, 1);
    QTableWidgetItem *nameItem =
        ui->equipmentTableWidget->item(row, 2);
    QTableWidgetItem *categoryItem =
        ui->equipmentTableWidget->item(row, 3);
    QTableWidgetItem *conditionItem =
        ui->equipmentTableWidget->item(row, 4);

    if (!assetTagItem || !nameItem ||
        !categoryItem || !conditionItem) {
        return;
    }

    ui->assetTagLineEdit->setText(assetTagItem->text());
    ui->equipmentNameLineEdit->setText(nameItem->text());
    ui->categoryComboBox->setCurrentText(categoryItem->text());
    ui->conditionComboBox->setCurrentText(conditionItem->text());
}

void MainWindow::on_updateEquipmentButton_clicked()
{
    int selectedRow =
        ui->equipmentTableWidget->currentRow();

    if (selectedRow < 0) {
        QMessageBox::warning(
            this,
            "No Equipment Selected",
            "Please select an equipment item from the table."
            );
        return;
    }

    QTableWidgetItem *idItem =
        ui->equipmentTableWidget->item(selectedRow, 0);

    if (!idItem) {
        return;
    }

    QString assetTag =
        ui->assetTagLineEdit->text().trimmed();
    QString equipmentName =
        ui->equipmentNameLineEdit->text().trimmed();
    QString category =
        ui->categoryComboBox->currentText();
    QString condition =
        ui->conditionComboBox->currentText();

    if (assetTag.isEmpty() || equipmentName.isEmpty()) {
        QMessageBox::warning(
            this,
            "Missing Information",
            "Please enter an asset tag and equipment name."
            );
        return;
    }

    QSqlQuery query(database);

    query.prepare(
        "UPDATE Equipment "
        "SET serial_number = :assetTag, "
        "name = :name, "
        "category = :category, "
        "condition = :condition "
        "WHERE equipment_id = :equipmentId"
        );

    query.bindValue(":assetTag", assetTag);
    query.bindValue(":name", equipmentName);
    query.bindValue(":category", category);
    query.bindValue(":condition", condition);
    query.bindValue(
        ":equipmentId",
        idItem->text().toInt()
        );

    if (!query.exec()) {
        QMessageBox::critical(
            this,
            "Update Error",
            "The equipment could not be updated:\n" +
                query.lastError().text()
            );
        return;
    }

    QMessageBox::information(
        this,
        "Equipment Updated",
        "The equipment was updated successfully."
        );

    loadEquipment();

    ui->assetTagLineEdit->clear();
    ui->equipmentNameLineEdit->clear();
    ui->categoryComboBox->setCurrentIndex(0);
    ui->conditionComboBox->setCurrentIndex(0);
    ui->equipmentTableWidget->clearSelection();
}

void MainWindow::on_searchEquipmentButton_clicked()
{
    QString searchText =
        ui->equipmentSearchLineEdit->text().trimmed();

    if (searchText.isEmpty()) {
        loadEquipment();
        return;
    }

    ui->equipmentTableWidget->setRowCount(0);

    QSqlQuery query(database);

    query.prepare(
        "SELECT equipment_id, serial_number, name, category, "
        "condition, status "
        "FROM Equipment "
        "WHERE serial_number LIKE :search "
        "OR name LIKE :search "
        "OR category LIKE :search "
        "OR condition LIKE :search "
        "OR status LIKE :search "
        "ORDER BY equipment_id"
        );

    query.bindValue(":search", "%" + searchText + "%");

    if (!query.exec()) {
        QMessageBox::critical(
            this,
            "Search Error",
            "The equipment search could not be completed:\n" +
                query.lastError().text()
            );
        return;
    }

    while (query.next()) {
        int row = ui->equipmentTableWidget->rowCount();
        ui->equipmentTableWidget->insertRow(row);

        for (int column = 0; column < 6; column++) {
            ui->equipmentTableWidget->setItem(
                row,
                column,
                new QTableWidgetItem(
                    query.value(column).toString()
                    )
                );
        }
    }
}

void MainWindow::on_showAllEquipmentButton_clicked()
{
    ui->equipmentSearchLineEdit->clear();
    loadEquipment();
}

void MainWindow::on_addBorrowerButton_clicked()
{
    QString name =
        ui->borrowerNameLineEdit->text().trimmed();
    QString email =
        ui->borrowerEmailLineEdit->text().trimmed();
    QString phone =
        ui->borrowerPhoneLineEdit->text().trimmed();

    if (name.isEmpty()) {
        QMessageBox::warning(
            this,
            "Missing Information",
            "Please enter the borrower's name."
            );
        return;
    }

    QSqlQuery query(database);

    query.prepare(
        "INSERT INTO Borrowers (name, email, phone) "
        "VALUES (:name, :email, :phone)"
        );

    query.bindValue(":name", name);
    query.bindValue(":email", email);
    query.bindValue(":phone", phone);

    if (!query.exec()) {
        QMessageBox::critical(
            this,
            "Borrower Error",
            "The borrower could not be added:\n" +
                query.lastError().text()
            );
        return;
    }

    QMessageBox::information(
        this,
        "Borrower Added",
        "The borrower was added successfully."
        );

    ui->borrowerNameLineEdit->clear();
    ui->borrowerEmailLineEdit->clear();
    ui->borrowerPhoneLineEdit->clear();

    loadBorrowers();
}

void MainWindow::on_clearBorrowerButton_clicked()
{
    ui->borrowerNameLineEdit->clear();
    ui->borrowerEmailLineEdit->clear();
    ui->borrowerPhoneLineEdit->clear();
    ui->borrowerTableWidget->clearSelection();
    ui->borrowerNameLineEdit->setFocus();
}

void MainWindow::on_borrowerTableWidget_cellClicked(
    int row,
    int column
    )
{
    Q_UNUSED(column);

    QTableWidgetItem *nameItem =
        ui->borrowerTableWidget->item(row, 1);
    QTableWidgetItem *emailItem =
        ui->borrowerTableWidget->item(row, 2);
    QTableWidgetItem *phoneItem =
        ui->borrowerTableWidget->item(row, 3);

    if (!nameItem || !emailItem || !phoneItem) {
        return;
    }

    ui->borrowerNameLineEdit->setText(nameItem->text());
    ui->borrowerEmailLineEdit->setText(emailItem->text());
    ui->borrowerPhoneLineEdit->setText(phoneItem->text());
}

void MainWindow::on_updateBorrowerButton_clicked()
{
    int selectedRow =
        ui->borrowerTableWidget->currentRow();

    if (selectedRow < 0) {
        QMessageBox::warning(
            this,
            "No Borrower Selected",
            "Please select a borrower from the table."
            );
        return;
    }

    QTableWidgetItem *idItem =
        ui->borrowerTableWidget->item(selectedRow, 0);

    if (!idItem) {
        return;
    }

    QString name =
        ui->borrowerNameLineEdit->text().trimmed();
    QString email =
        ui->borrowerEmailLineEdit->text().trimmed();
    QString phone =
        ui->borrowerPhoneLineEdit->text().trimmed();

    if (name.isEmpty()) {
        QMessageBox::warning(
            this,
            "Missing Information",
            "Please enter the borrower's name."
            );
        return;
    }

    QSqlQuery query(database);

    query.prepare(
        "UPDATE Borrowers "
        "SET name = :name, email = :email, phone = :phone "
        "WHERE borrower_id = :borrowerId"
        );

    query.bindValue(":name", name);
    query.bindValue(":email", email);
    query.bindValue(":phone", phone);
    query.bindValue(
        ":borrowerId",
        idItem->text().toInt()
        );

    if (!query.exec()) {
        QMessageBox::critical(
            this,
            "Update Error",
            "The borrower could not be updated:\n" +
                query.lastError().text()
            );
        return;
    }

    QMessageBox::information(
        this,
        "Borrower Updated",
        "The borrower was updated successfully."
        );

    loadBorrowers();

    ui->borrowerNameLineEdit->clear();
    ui->borrowerEmailLineEdit->clear();
    ui->borrowerPhoneLineEdit->clear();
    ui->borrowerTableWidget->clearSelection();
}

void MainWindow::on_showAllBorrowersButton_clicked()
{
    ui->borrowerSearchLineEdit->clear();
    loadBorrowers();
}

void MainWindow::on_searchBorrowerButton_clicked()
{
    QString searchText =
        ui->borrowerSearchLineEdit->text().trimmed();

    if (searchText.isEmpty()) {
        loadBorrowers();
        return;
    }

    ui->borrowerTableWidget->setRowCount(0);

    QSqlQuery query(database);

    query.prepare(
        "SELECT borrower_id, name, email, phone "
        "FROM Borrowers "
        "WHERE name LIKE :search "
        "OR email LIKE :search "
        "OR phone LIKE :search "
        "ORDER BY borrower_id"
        );

    query.bindValue(":search", "%" + searchText + "%");

    if (!query.exec()) {
        QMessageBox::critical(
            this,
            "Search Error",
            "The borrower search could not be completed:\n" +
                query.lastError().text()
            );
        return;
    }

    while (query.next()) {
        int row = ui->borrowerTableWidget->rowCount();
        ui->borrowerTableWidget->insertRow(row);

        for (int column = 0; column < 4; column++) {
            ui->borrowerTableWidget->setItem(
                row,
                column,
                new QTableWidgetItem(
                    query.value(column).toString()
                    )
                );
        }
    }
}
