/**
 *    Copyright (C) 2020-present MongoDB, Inc.
 *
 *    This program is free software: you can redistribute it and/or modify
 *    it under the terms of the Server Side Public License, version 1,
 *    as published by MongoDB, Inc.
 *
 *    This program is distributed in the hope that it will be useful,
 *    but WITHOUT ANY WARRANTY; without even the implied warranty of
 *    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *    Server Side Public License for more details.
 *
 *    You should have received a copy of the Server Side Public License
 *    along with this program. If not, see
 *    <http://www.mongodb.com/licensing/server-side-public-license>.
 *
 *    As a special exception, the copyright holders give permission to link the
 *    code of portions of this program with the OpenSSL library under certain
 *    conditions as described in each individual source file and distribute
 *    linked combinations including the program with the OpenSSL library. You
 *    must comply with the Server Side Public License in all respects for
 *    all of the code used other than as permitted herein. If you modify file(s)
 *    with this exception, you may extend this exception to your version of the
 *    file(s), but you are not obligated to do so. If you do not wish to do so,
 *    delete this exception statement from your version. If you delete this
 *    exception statement from all source files in the program, then also delete
 *    it in the license file.
 */
#define MONGO_LOGV2_DEFAULT_COMPONENT ::mongo::logv2::LogComponent::kReplication;


#include "mongo/db/commands/migrate_tenant_cmds.h"
#include "mongo/db/commands/migrate_tenant_cmds_gen.h"
#include "mongo/db/repl/migrate_tenant_state_machine_gen.h"
#include "mongo/db/repl/migrating_tenant_donor_util.h"
#include "mongo/logv2/log.h"

namespace mongo {
namespace {


class DonorStartMigrationCmd : public MigrationDonorCmdBase<DonorStartMigrationCmd> {
public:
    using Request = DonorStartMigration;
    using ParentInvocation = MigrationDonorCmdBase<DonorStartMigrationCmd>::Invocation;
    class Invocation : public ParentInvocation {
        using ParentInvocation::ParentInvocation;

    public:
        void typedRun(OperationContext* opCtx) {
            const auto requestBody = request();
            auto donorDocument = getDonorDocumentFromRequest(requestBody);

            migrating_tenant_donor_util::persistDonorStateDocument(opCtx, donorDocument);
            migrating_tenant_donor_util::dataSync(opCtx, donorDocument);
        }

        TenantMigrationDonorDocument getDonorDocumentFromRequest(const RequestType& requestBody) {
            mongo::UUID migrationId = requestBody.getMigrationId();

            std::string recipientURI = requestBody.getRecipientConnectionString().toString();
            std::string dbPrefix = requestBody.getDatabasePrefix().toString();

            auto donorStartState = TenantMigrationDonorStateEnum::kDataSync;
            bool garbageCollect = false;
            const TenantMigrationDonorDocument donorDocument(
                OID::gen(), migrationId, recipientURI, dbPrefix, donorStartState, garbageCollect);

            return donorDocument;
        }

    private:
        void doCheckAuthorization(OperationContext* opCtx) const {}
    };

    std::string help() const {
        return "Start migrating databases whose names match the specified prefix to the specified "
               "replica set.";
    }

} donorStartMigrationCmd;

class DonorWaitForMigrationToCommitCmd
    : public MigrationDonorCmdBase<DonorWaitForMigrationToCommitCmd> {
public:
    using Request = DonorWaitForMigrationToCommit;
    using ParentInvocation = MigrationDonorCmdBase<DonorWaitForMigrationToCommitCmd>::Invocation;
    class Invocation : public ParentInvocation {
        using ParentInvocation::ParentInvocation;

    public:
        void typedRun(OperationContext* opCtx) {}

    private:
        void doCheckAuthorization(OperationContext* opCtx) const {}
    };

    std::string help() const override {
        return "Wait for migration to be commited.";
    }

} donorWaitForMigrationToCommit;

class DonorForgetMigrationCmd : public MigrationDonorCmdBase<DonorForgetMigrationCmd> {
public:
    using Request = DonorForgetMigration;
    using ParentInvocation = MigrationDonorCmdBase<DonorForgetMigrationCmd>::Invocation;
    class Invocation : public ParentInvocation {
        using ParentInvocation::ParentInvocation;

    public:
        void typedRun(OperationContext* opCtx) {}

    private:
        void doCheckAuthorization(OperationContext* opCtx) const {}
    };

    std::string help() const override {
        return "Forget a migration";
    }
} donorForgetMigration;

}  // namespace
}  // namespace mongo