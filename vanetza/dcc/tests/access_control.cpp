#include <gtest/gtest.h>
#include <vanetza/access/data_request.hpp>
#include <vanetza/access/interface.hpp>
#include <vanetza/dcc/access_control.hpp>
#include <vanetza/dcc/scheduler.hpp>
#include <vanetza/dcc/state_machine.hpp>

using namespace vanetza;
using namespace vanetza::dcc;

class FakeAccessInterface : public access::Interface
{
public:
    void request(const access::DataRequest& req, std::unique_ptr<ChunkPacket>) override
    {
        last_request = req;
    }

    boost::optional<access::DataRequest> last_request;
};

class AccessControlTest : public ::testing::Test
{
protected:
    AccessControlTest() : sc(fsm, now), ctrl(sc, ifc) {}

    clock::time_point now;
    StateMachine fsm;
    Scheduler sc;
    FakeAccessInterface ifc;
    AccessControl ctrl;
};


TEST_F(AccessControlTest, request_mapping_voice)
{
    DataRequest request;
    request.dcc_profile = Profile::DP0;
    ctrl.request(request, nullptr);
    EXPECT_EQ(AccessCategory::VO, ifc.last_request->access_category);
}

TEST_F(AccessControlTest, request_mapping_video)
{
    DataRequest request;
    request.dcc_profile = Profile::DP1;
    ctrl.request(request, nullptr);
    EXPECT_EQ(AccessCategory::VI, ifc.last_request->access_category);
}

TEST_F(AccessControlTest, request_mapping_best_effort)
{
    DataRequest request;
    request.dcc_profile = Profile::DP2;
    ctrl.request(request, nullptr);
    EXPECT_EQ(AccessCategory::BE, ifc.last_request->access_category);
}

TEST_F(AccessControlTest, request_mapping_background)
{
    DataRequest request;
    request.dcc_profile = Profile::DP3;
    ctrl.request(request, nullptr);
    EXPECT_EQ(AccessCategory::BK, ifc.last_request->access_category);
}

TEST_F(AccessControlTest, dropping)
{
    DataRequest request;
    request.dcc_profile = Profile::DP0;

    for (unsigned i = 0; i < 20; ++i) {
        ifc.last_request.reset();
        ASSERT_FALSE(!!ifc.last_request);
        ctrl.request(request, nullptr);
        EXPECT_TRUE(!!ifc.last_request);
    }

    ifc.last_request.reset();
    ctrl.request(request, nullptr);
    EXPECT_FALSE(!!ifc.last_request);
}

TEST_F(AccessControlTest, scheduler_notification)
{
    ASSERT_EQ(std::chrono::milliseconds(0), sc.delay(Profile::DP2));
    DataRequest request;
    request.dcc_profile = Profile::DP2;
    ctrl.request(request, nullptr);
    EXPECT_LT(std::chrono::milliseconds(0), sc.delay(Profile::DP2));
}

TEST(AccessControl, map_profile_onto_ac)
{
    EXPECT_EQ(AccessCategory::VO, map_profile_onto_ac(Profile::DP0));
    EXPECT_EQ(AccessCategory::VI, map_profile_onto_ac(Profile::DP1));
    EXPECT_EQ(AccessCategory::BE, map_profile_onto_ac(Profile::DP2));
    EXPECT_EQ(AccessCategory::BK, map_profile_onto_ac(Profile::DP3));

    auto malicious_profile = static_cast<Profile>(4);
    EXPECT_THROW(map_profile_onto_ac(malicious_profile), std::invalid_argument);
}
