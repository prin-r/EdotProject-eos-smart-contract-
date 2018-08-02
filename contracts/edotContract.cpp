#include <utility>
#include <vector>
#include <string>
#include <eosiolib/eosio.hpp>
#include <eosiolib/time.hpp>
#include <eosiolib/asset.hpp>
#include <eosiolib/contract.hpp>
#include <eosiolib/crypto.h>

using eosio::key256;
using eosio::indexed_by;
using eosio::const_mem_fun;
using eosio::asset;
using eosio::permission_level;
using eosio::action;
using eosio::print;
using eosio::name;

using namespace eosio;
using namespace std;

class edotContract : public eosio::contract {
    public:

        edotContract(account_name self)
        :eosio::contract(self),
        devices(_self, _self),
        records(_self,_self)
        {}

        ///@abi action
        void deposit( uint64_t dev_id, const account_name from, const asset& quantity ) {

            require_auth(from);
            
            eosio_assert( quantity.is_valid(), "invalid quantity" );
            eosio_assert( quantity.amount > 0, "must deposit positive quantity" );

            auto itr = devices.find(dev_id);
            eosio_assert( itr != devices.end() , "device not found" );
            eosio_assert( itr->owner == from , "device permission denie" );

            action(
                permission_level{ from, N(active) },
                N(eosio.token), N(transfer),
                std::make_tuple(from, _self, quantity, string(""))
            ).send();

            devices.modify( itr, 0, [&]( auto& dev ) {
                dev.eos_balance += quantity;
            });
        }
        ///@abi action
        void addrecord(uint64_t dev_id, uint64_t id, int64_t longtitude, int64_t latitude, int64_t temp) {

            auto itr_rec = records.find(id);
            eosio_assert( itr_rec == records.end() , "already have this record id" );

            auto itr_dev = devices.find(dev_id);
            eosio_assert( itr_dev != devices.end() , "device not found" );

            records.emplace(_self, [&](auto& p) {
                p.id = id;
                p.time_stamp = eosio::time_point_sec(now());
                p.device_id = dev_id;
                p.longtitude = longtitude;
                p.latitude = latitude;
                p.temp = temp;
            });

            print("record#",id,"was added");
        }


        ///@abi action
        void registerdev(const account_name from, const uint64_t id,string device_name, string secret, uint32_t period)  {

            eosio_assert( period > 0, "period should more than 0" );

            auto itr = devices.find(id);
            eosio_assert( itr == devices.end() , "already have this device id" );

            devices.emplace(_self, [&](auto& p){
                p.id = id;
                p.device_name = device_name;
                p.secret = secret;
                p.status = 0;
                p.owner = from;
                p.time_stamp = eosio::time_point_sec(now());
            });

            print("register#",id," created123");

        }


    private:
        ///@abi table
        struct device {
            uint64_t    id;
            uint64_t    status;    // 0: not available 1: available
            string      device_name;
            string      secret;
            asset       eos_balance;   // support only EOS
            account_name owner;
            eosio::time_point_sec   time_stamp;

            uint64_t get_id() const {return id;}
            uint64_t get_status() const {return status;}
            string get_device_name() const {return device_name;}
            asset get_balance() const {return eos_balance;}
            uint64_t primary_key() const { return id; }

            EOSLIB_SERIALIZE( device, (id)(status)(device_name)(secret)(eos_balance)(owner)(time_stamp) )
        };

        eosio::multi_index< N(device), device> devices;

        struct record {
            eosio::time_point_sec   time_stamp;
            uint64_t                id;
            int64_t                 longtitude;
            int64_t                 latitude;
            int64_t                 temp;
            uint64_t                device_id;

            eosio::time_point_sec get_time_stamp() const {return time_stamp;}
            int64_t get_longtitude() const {return longtitude;}
            int64_t get_latitude() const {return latitude;}
            int64_t get_temp() const {return temp;}
            uint64_t primary_key() const { return id; }

            EOSLIB_SERIALIZE( record, (time_stamp)(longtitude)(latitude)(temp)(device_id)(id))
        };

        eosio::multi_index< N(record), record> records;


};

EOSIO_ABI( edotContract, (deposit)(registerdev)(addrecord) )