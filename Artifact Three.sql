--Tyler Beckham
--DAD 220
--Final Project Code

--Task 1: Insert Record to the Person Table
mysql> INSERT INTO person (first_name, last_name)
    -> VALUES ("Tyler", "Beckham");
    
--Task 2: Alter the Person Table
mysql> ALTER TABLE person ADD hair_color VARCHAR(25);

--Task 3: Update Records in the Person Table
mysql> UPDATE person
    -> SET hair_color = "Brown"
    -> WHERE last_name = "Beckham";

--Task 4: Delete Records from Person Table
mysql> DELETE FROM person WHERE first_name = "Diana" AND last_name = "Taurasi";

--Task 5: Alter the Contact List Table
mysql> ALTER TABLE contact_list ADD favorite VARCHAR(10);

--Task 6: Update Records in the Contact List Table
mysql> UPDATE contact_list
    -> SET favorite = "y"
    -> WHERE contact_id = 1;

--Task 7: Update Records in the Contact List Table
mysql> UPDATE contact_list
    -> SET favorite = "n"
    -> WHERE contact_id <> 1;

--Task 8: Insert Records to Contact List Table
mysql> INSERT INTO contact_list (person_id, contact_id, favorite)
    -> VALUES
    -> (7,1,"y"),
    -> (7,4,"n"),
    -> (7,2,"n");

--Task 9: Create the Image Table
mysql> CREATE TABLE image (
    -> image_id INT(8) AUTO_INCREMENT NOT NULL,
    -> image_name VARCHAR(50) NOT NULL,
    -> image_location VARCHAR(250) NOT NULL,
    -> PRIMARY KEY (image_id)
    -> ) AUTO_INCREMENT = 1;

--Task 10: Create the Message-Image Intersection Table
mysql> CREATE TABLE message_image (
    -> message_id INT(8) NOT NULL,
    -> image_id INT(8) NOT NULL,
    -> PRIMARY KEY (message_id, image_id)
    -> );

--Task 11: Insert Records to Image Table
mysql> INSERT INTO image (image_name, image_location)
    -> VALUES
    -> ("Dogs","Central Park"),
    -> ("Beach","Folly Beach"),
    -> ("Home","South Carolina"),
    -> ("Car","South Carolina"),
    -> ("Vacation","New York");

--Task 12: Insert Records to Message_Image Table
mysql> INSERT INTO message_image (message_id, image_id)
    -> VALUES
    -> (5,5),
    -> (2,2),
    -> (2,4),
    -> (4,1),
    -> (3,3);

--Task 13: Find All of the Messages that Michael Phelps Sent
mysql> SELECT
    -> pSender.first_name AS "Sender\'s first name",
    -> pSender.last_name AS "Sender\'s last name",
    -> pReceiver.first_name AS "Receiver\'s first name",
    -> pReceiver.last_name AS "Receiver\'s last name",
    -> m.message_id AS "Message ID",
    -> m.message AS "Message",
    -> m.send_datetime AS "Message Timestamp"
    -> FROM
    -> message m
    -> JOIN person pSender ON pSender.person_id = m.sender_id
    -> JOIN person pReceiver ON pReceiver.person_id = m.receiver_id
    -> WHERE sender_id = 1;

--Task 14: Find the Number of Messages Sent for Every Person
mysql> SELECT COUNT(message.sender_id) AS "Count of messages",person.person_id AS "Person ID",
    -> person.first_name AS "First Name", person.last_name AS "Last Name"
    -> FROM message
    -> JOIN person ON person.person_id = message.sender_id
    -> WHERE person.person_id > 0
    -> GROUP BY person.person_id;

--Task 15: Find All of the Messages that Have At Least One Image Attached Using INNER JOINs
mysql> SELECT message_image.message_id AS "Message ID", message.message AS "Message",
    -> message.send_datetime AS "Message Timestamp", image.image_name AS "First Image Name",
    -> image.image_location AS "First Image Location"
    -> FROM message_image
    -> JOIN message ON message_image.message_id = message.message_id
    -> JOIN image ON message_image.image_id = image.image_id
    -> GROUP BY message_image.message_id;
    
--New Task: Create a stored procedure to easily be able to call task 13, 14, and 15.
mysql> CREATE PROCEDURE AllMessagesMichaelPhelpsSent
    -> AS
    -> SELECT
    -> pSender.first_name AS "Sender\'s first name",
    -> pSender.last_name AS "Sender\'s last name",
    -> pReceiver.first_name AS "Receiver\'s first name",
    -> pReceiver.last_name AS "Receiver\'s last name",
    -> m.message_id AS "Message ID",
    -> m.message AS "Message",
    -> m.send_datetime AS "Message Timestamp"
    -> FROM
    -> message m
    -> JOIN person pSender ON pSender.person_id = m.sender_id
    -> JOIN person pReceiver ON pReceiver.person_id = m.receiver_id
    -> WHERE sender_id = 1;
    -> GO;
    
mysql> CREATE PROCEDURE NumberOfMessagesEveryoneSent
    -> AS
    -> SELECT COUNT(message.sender_id) AS "Count of messages",person.person_id AS "Person ID",
    -> person.first_name AS "First Name", person.last_name AS "Last Name"
    -> FROM message
    -> JOIN person ON person.person_id = message.sender_id
    -> WHERE person.person_id > 0
    -> GROUP BY person.person_id;
    -> GO;
    
mysql> CREATE PROCEDURE AllMessagesWithOneOrMoreImage
    -> AS
    -> SELECT message_image.message_id AS "Message ID", message.message AS "Message",
    -> message.send_datetime AS "Message Timestamp", image.image_name AS "First Image Name",
    -> image.image_location AS "First Image Location"
    -> FROM message_image
    -> JOIN message ON message_image.message_id = message.message_id
    -> JOIN image ON message_image.image_id = image.image_id
    -> GROUP BY message_image.message_id;
    -> GO;
    
--Calling the Stored Procedures
mysql> EXEC AllMessagesMichaelPhelpsSent;

mysql> EXEC NumberOfMessagesEveryoneSent;

mysql> EXEC AllMessagesWithOneOrMoreImage;