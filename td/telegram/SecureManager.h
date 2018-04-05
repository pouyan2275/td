//
// Copyright Aliaksei Levin (levlam@telegram.org), Arseny Smirnov (arseny30@gmail.com) 2014-2018
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//
#pragma once
#include "td/actor/actor.h"

#include "td/telegram/net/NetQuery.h"
#include "td/telegram/SecureValue.h"
#include "td/telegram/files/FileManager.h"

#include "td/telegram/td_api.h"

#include "td/utils/optional.h"

namespace td {
using TdApiSecureValue = td_api::object_ptr<td_api::passportData>;
class GetSecureValue : public NetQueryCallback {
 public:
  GetSecureValue(ActorShared<> parent, std::string password, SecureValueType type, Promise<TdApiSecureValue> promise);

 private:
  ActorShared<> parent_;
  string password_;
  SecureValueType type_;
  Promise<TdApiSecureValue> promise_;
  optional<EncryptedSecureValue> encrypted_secure_value_;
  optional<secure_storage::Secret> secret_;

  void on_error(Status status);
  void on_secret(Result<secure_storage::Secret> r_secret, bool dummy);
  void loop() override;
  void start_up() override;

  void on_result(NetQueryPtr query) override;
};

class SetSecureValue : public NetQueryCallback {
 public:
  SetSecureValue(ActorShared<> parent, string password, SecureValue secure_value, Promise<TdApiSecureValue> promise);

 private:
  ActorShared<> parent_;
  string password_;
  SecureValue secure_value_;
  Promise<TdApiSecureValue> promise_;
  optional<secure_storage::Secret> secret_;

  size_t files_left_to_upload_ = 0;
  vector<SecureInputFile> to_upload_;
  class UploadCallback;
  std::shared_ptr<UploadCallback> upload_callback_;

  enum class State { WaitSecret, WaitSetValue } state_ = State::WaitSecret;

  class UploadCallback : public FileManager::UploadCallback {
   public:
    explicit UploadCallback(ActorId<SetSecureValue> actor_id);

   private:
    ActorId<SetSecureValue> actor_id_;
    void on_upload_ok(FileId file_id, tl_object_ptr<telegram_api::InputFile> input_file) override;
    void on_upload_encrypted_ok(FileId file_id, tl_object_ptr<telegram_api::InputEncryptedFile> input_file) override;
    void on_upload_secure_ok(FileId file_id, tl_object_ptr<telegram_api::InputSecureFile> input_file) override;
    void on_upload_error(FileId file_id, Status error) override;
  };

  void on_upload_ok(FileId file_id, tl_object_ptr<telegram_api::InputSecureFile> input_file);
  void on_upload_error(FileId file_id, Status error);

  void on_error(Status status);

  void on_secret(Result<secure_storage::Secret> r_secret, bool x);

  void start_up() override;

  void loop() override;
  void on_result(NetQueryPtr query) override;
};

class SecureManager : public Actor {
 public:
  SecureManager(ActorShared<> parent);

  void get_secure_value(std::string password, SecureValueType type, Promise<TdApiSecureValue> promise);
  void set_secure_value(string password, SecureValue secure_value, Promise<TdApiSecureValue> promise);

 private:
  ActorShared<> parent_;
  int32 refcnt_{1};
  std::map<SecureValueType, ActorOwn<>> set_secure_value_queries_;

  void hangup() override;
  void hangup_shared() override;
  void dec_refcnt();
};
}  // namespace td