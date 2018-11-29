'use strict';

var path = require( 'path' );

const CdrPlugin = require( path.join( global.CDR_INSTALL_DIR, '/commander/classes/CdrPlugin' ) ).CdrPlugin;

module.exports = class CfeCdrPlugin extends CdrPlugin {
  constructor( urlBase ) {
    super( 'ms5611', path.join( __dirname, 'web', urlBase ) );
  }

  getContent() {
    var result = {
      shortDescription: 'MS5611',
      longDescription: 'MS5611 Barometric Altimeter Driver.',
      nodes: {
        main: {
          type: CdrPlugin.ContentType.LAYOUT,
          shortDescription: 'Main',
          longDescription: 'Main MS5611.',
          filePath: '/main.lyt'
        },
        cdh: {
          type: CdrPlugin.ContentType.PANEL,
          shortDescription: 'Command and Data Handling',
          longDescription: 'Command counters.',
          filePath: '/cdh.pug'
        },
        cfg: {
          type: CdrPlugin.ContentType.PANEL,
          shortDescription: 'Configuration',
          longDescription: 'Configuration Diagnostic Message.',
          filePath: '/cfg.pug'
        },
        meas: {
          type: CdrPlugin.ContentType.PANEL,
          shortDescription: 'Measurement',
          longDescription: 'Measurement Diagnostic Message.',
          filePath: '/meas.pug'
        }
      }
    };

    return result;
  }
};

//var plugin = new CfeCdrPlugin();

//let CfeCdrPlugin = new class extends CdrPlugin {