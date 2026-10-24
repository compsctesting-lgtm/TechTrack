#include "mainwindow.h"
#include "ui_mainwindow.h"

#include <QMessageBox>
#include <QSqlError>
#include <QSqlQuery>
#include <QStatusBar>
#include <QStringList>

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
{
    ui->setupUi(this);

    if (setupDatabase()) {
        statusBar()->showMessage("Database connected successfully.");
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

    return true;
}