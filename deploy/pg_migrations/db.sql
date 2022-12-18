CREATE TABLE IF NOT EXISTS message_storage(
    id SERIAL PRIMARY KEY,
    sender TEXT,
    receiver TEXT,
    all_receivers TEXT,
    send_time INTEGER,
    message TEXT,
    reply TEXT
);

CREATE INDEX idx__receiver__send_time
ON message_storage(receiver, send_time);

CREATE INDEX idx__sender__send_time
ON message_storage(sender, send_time);
