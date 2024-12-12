#include <asx/i2c_slave.hpp>

namespace asx {
    namespace i2c {
        /*! \brief Initializes TWI slave driver structure.
        *
        *  Initialize the instance of the TWI Slave and set the appropriate values.
        *
        *  \param twi                  The TWI_Slave_t struct instance.
        *  \param module               Pointer to the TWI module.
        *  \param processDataFunction  Pointer to the function that handles incoming data.
        */
        void Slave::initialize_driver( void (*processDataFunction) (void) )
        {
            Process_Data = processDataFunction;
            bytesReceived = 0;
            bytesSent = 0;
            status = TWIS_STATUS_READY;
            result = TWIS_RESULT_UNKNOWN;
            abort = false;
        }


        /*! \brief Initialize the TWI module.
        *
        *  Enables interrupts on address recognition and data available.
        *  Remember to enable interrupts globally from the main application.
        *
        *  \param twi        The TWI_Slave_t struct instance.
        *  \param address    Slave address for this module.
        *  \param intLevel   Interrupt level for the TWI slave interrupt handler.
        */
        void TWI_SlaveInitializeModule(TWI_Slave_t *twi,
                                    uint8_t address)
        {
            twi->interface->SCTRLA =      TWI_DIEN_bm |
                                        TWI_APIEN_bm |
                                        TWI_ENABLE_bm;
            twi->interface->SADDR = (address<<1);
        }


        /*! \brief Common TWI slave interrupt service routine.
        *
        *  Handles all TWI transactions and responses to address match, data reception,
        *  data transmission, bus error and data collision.
        *
        *  \param twi The TWI_Slave_t struct instance.
        */
        void Slave::interrupt_handler()
        {
            uint8_t currentStatus = twi->interface->SSTATUS;

            /* If bus error. */
            if (currentStatus & TWI_BUSERR_bm) {
                twi->bytesReceived = 0;
                twi->bytesSent = 0;
                twi->result = TWIS_RESULT_BUS_ERROR;
                twi->status = TWIS_STATUS_READY;
            }

            /* If transmit collision. */
            else if (currentStatus & TWI_COLL_bm) {
                twi->bytesReceived = 0;
                twi->bytesSent = 0;
                twi->result = TWIS_RESULT_TRANSMIT_COLLISION;
                twi->status = TWIS_STATUS_READY;
            }

            /* If address match. */
            else if ((currentStatus & TWI_APIF_bm) &&
                    (currentStatus & TWI_AP_bm)) {

                TWI_SlaveAddressMatchHandler(twi);
            }

            /* If stop (only enabled through slave read transaction). */
            else if (currentStatus & TWI_APIF_bm) {
                TWI_SlaveStopHandler(twi);
            }

            /* If data interrupt. */
            else if (currentStatus & TWI_DIF_bm) {
                TWI_SlaveDataHandler(twi);
            }

            /* If unexpected state. */
            else {
                TWI_SlaveTransactionFinished(twi, TWIS_RESULT_FAIL);
            }
        }

        /*! \brief TWI address match interrupt handler.
        *
        *  Prepares TWI module for transaction when an address match occurs.
        *
        *  \param twi The TWI_Slave_t struct instance.
        */
        void Slave::AddressMatchHandler(TWI_Slave_t *twi)
        {
            /* If application signaling need to abort (error occured). */
            if (twi->abort) {
                twi->interface->SCTRLB = TWI_SCMD_COMPTRANS_gc;
                TWI_SlaveTransactionFinished(twi, TWIS_RESULT_ABORTED);
                twi->abort = false;
            } else {
                twi->status = TWIS_STATUS_BUSY;
                twi->result = TWIS_RESULT_UNKNOWN;

                /* Disable stop interrupt. */
                uint8_t currentCtrlA = twi->interface->SCTRLA;
                twi->interface->SCTRLA = currentCtrlA & ~TWI_PIEN_bm;

                twi->bytesReceived = 0;
                twi->bytesSent = 0;

                /* Send ACK, wait for data interrupt. */
                twi->interface->SCTRLB = TWI_SCMD_RESPONSE_gc;
            }
        }


        /*! \brief TWI stop condition interrupt handler.
        *
        *  \param twi The TWI_Slave_t struct instance.
        */
        void Slave::StopHandler(TWI_Slave_t *twi)
        {
            /* Disable stop interrupt. */
            uint8_t currentCtrlA = twi->interface->SCTRLA;
            twi->interface->SCTRLA = currentCtrlA & ~TWI_PIEN_bm;

            /* Clear APIF, according to flowchart don't ACK or NACK */
            uint8_t currentStatus = twi->interface->SSTATUS;
            twi->interface->SSTATUS = currentStatus | TWI_APIF_bm;

            TWI_SlaveTransactionFinished(twi, TWIS_RESULT_OK);

        }


        /*! \brief TWI data interrupt handler.
        *
        *  Calls the appropriate slave read or write handler.
        *
        *  \param twi The TWI_Slave_t struct instance.
        */
        void Slave::DataHandler(TWI_Slave_t *twi)
        {
            if (twi->interface->SSTATUS & TWI_DIR_bm) {
                TWI_SlaveWriteHandler(twi);
            } else {
                TWI_SlaveReadHandler(twi);
            }
        }


        /*! \brief TWI slave read interrupt handler.
        *
        *  Handles TWI slave read transactions and responses.
        *
        *  \param twi The TWI_Slave_t struct instance.
        */
        void Slave::ReadHandler(TWI_Slave_t *twi)
        {
            /* Enable stop interrupt. */
            uint8_t currentCtrlA = twi->interface->SCTRLA;
            twi->interface->SCTRLA = currentCtrlA | TWI_PIEN_bm;

            /* If free space in buffer. */
            if (twi->bytesReceived < TWIS_RECEIVE_BUFFER_SIZE) {
                /* Fetch data */
                uint8_t data = twi->interface->SDATA;
                twi->receivedData[twi->bytesReceived] = data;

                /* Process data. */
                twi->Process_Data();

                twi->bytesReceived++;

                /* If application signaling need to abort (error occurred),
                * complete transaction and wait for next START. Otherwise
                * send ACK and wait for data interrupt.
                */
                if (twi->abort) {
                    twi->interface->SCTRLB = TWI_SCMD_COMPTRANS_gc;
                    TWI_SlaveTransactionFinished(twi, TWIS_RESULT_ABORTED);
                    twi->abort = false;
                } else {
                    twi->interface->SCTRLB = TWI_SCMD_RESPONSE_gc;
                }
            }
        
            /* If buffer overflow, send NACK and wait for next START. Set
            * result buffer overflow.
            */
            else {
                twi->interface->SCTRLB = TWI_ACKACT_bm |
                                            TWI_SCMD_COMPTRANS_gc;
                TWI_SlaveTransactionFinished(twi, TWIS_RESULT_BUFFER_OVERFLOW);
            }
        }


        /*! \brief TWI slave write interrupt handler.
        *
        *  Handles TWI slave write transactions and responses.
        *
        *  \param twi The TWI_Slave_t struct instance.
        */
        void TWI_SlaveWriteHandler(TWI_Slave_t *twi)
        {
            /* If NACK, slave write transaction finished. */
            if ((twi->bytesSent > 0) && (twi->interface->SSTATUS &
                                        TWI_RXACK_bm)) {

                twi->interface->SCTRLB = TWI_SCMD_COMPTRANS_gc;
                TWI_SlaveTransactionFinished(twi, TWIS_RESULT_OK);
            }
            /* If ACK, master expects more data. */
            else {
                if (twi->bytesSent < TWIS_SEND_BUFFER_SIZE) {
                    uint8_t data = twi->sendData[twi->bytesSent];
                    twi->interface->SDATA = data;
                    twi->bytesSent++;

                    /* Send data, wait for data interrupt. */
                    twi->interface->SCTRLB = TWI_SCMD_RESPONSE_gc;
                }
                /* If buffer overflow. */
                else {
                    twi->interface->SCTRLB = TWI_SCMD_COMPTRANS_gc;
                    TWI_SlaveTransactionFinished(twi, TWIS_RESULT_BUFFER_OVERFLOW);
                }
            }
        }


        /*! \brief TWI transaction finished function.
        *
        *  Prepares module for new transaction.
        *
        *  \param twi    The TWI_Slave_t struct instance.
        *  \param result The result of the transaction.
        */
        void TWI_SlaveTransactionFinished(TWI_Slave_t *twi, uint8_t result)
        {
            twi->result = result;
            twi->status = TWIS_STATUS_READY;
        }

    }
}


ISR(TWI0_TWIS_vect)
{
   TWI_SlaveInterruptHandler(&slave);
}